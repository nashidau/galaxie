
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include <talloc.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Con.h>

#include "tpe.h"
#include "browser.h"
#include "tpe_board.h"
#include "tpe_comm.h"
#include "tpe_event.h"
#include "tpe_gui.h"
#include "server.h"
#include "tpe_obj.h"
#include "tpe_orders.h"
#include "tpe_resources.h"
#include "tpe_sequence.h"
#include "tpe_ship.h"
#include "tpe_util.h"

#define TPE_AI(name,desc,func)  extern struct ai *func(struct tpe *);
#include "ailist.h"
#undef TPE_AI

struct ai_info {
	const char *name;
	const char *description;
	struct ai *(*init)(struct tpe *);
} ai_info[] = {
#define TPE_AI(name,desc,func)  {name,desc,func},
#include "ailist.h"
#undef TPE_AI
	{ "none", "No AI - Don't use an AI.", NULL },
};
#define N_AIS  (sizeof(ai_info) / sizeof(ai_info[0]))


struct startopt {
	unsigned int browse :1;

	/* Server details */
	unsigned int http: 1;
	unsigned int ssl:  1;
	unsigned short port;
	char *server;
	
	char *game;

	char *username;
	char *password;
	
	/* AI options */
	struct ai_info *ai;

	/* GUI options */
	unsigned int usegui :1; 
	unsigned int fullscreen :1; 
	char *theme;

	struct {
		unsigned int messages : 1;
	} debug;

	int showoptions;
};

static struct startopt *parse_args(int argc, char **argv);
static int parse_usebrowser(struct startopt *opt, int i, char **args);
static int parse_test(struct startopt *opt, int i, char **args);
static int parse_username(struct startopt *opt, int i, char **args);
static int parse_password(struct startopt *opt, int i, char **args);
static int parse_server(struct startopt *opt, int i, char **args);
static int parse_port(struct startopt *opt, int i, char **args);
static int parse_url(struct startopt *opt, int i, char **args);
static int parse_game(struct startopt *opt, int i, char **args);
static int parse_ai(struct startopt *opt, int i, char **args);
static int parse_no_ai(struct startopt *opt, int i, char **args);
static int parse_list_ai(struct startopt *opt, int i, char **args);
static int parse_gui(struct startopt *opt, int i, char **args);
static int parse_fullscreen(struct startopt *opt, int i, char **args);
static int parse_theme(struct startopt *opt, int i, char **args);
static int parse_usage(struct startopt *opt, int i, char **args);
static int parse_options(struct startopt *opt, int i, char **args);
static int parse_debug_messages(struct startopt *opt, int i, char **args);
static const char *parse_option(char **args, int *i);
static void dump_options(struct startopt *);

static int tpe_regerror(int errcode, const regex_t *regex);

static struct args {
	const char *arg;
	int (*fn)(struct startopt *opt, int i, char **args);
} args[] = {
	{ "-b",		parse_usebrowser},
	{ "--user",	parse_username	},
	{ "-u",		parse_username	},
	{ "--password", parse_password	},
	{ "-p",		parse_password	},
	{ "--server",	parse_server	},
	{ "-s",		parse_server	},
	{ "--port",	parse_port	},
	{ "--game",	parse_game	},
	{ "-g",		parse_game	},
	{ "--ai",	parse_ai	},
	{ "--no-ai",	parse_no_ai	},
	{ "--noai",	parse_no_ai	},
	{ "--no-gui",   parse_gui	},
	{ "--nogui",    parse_gui	},
	{ "--theme",    parse_theme	},
	{ "-t",         parse_theme     },
	{ "--fullscreen", parse_fullscreen },
	{ "tp",		parse_url 	},
	{ "--options",  parse_options   },
	{ "--list-ais", parse_list_ai	},
	{ "--list-ai",  parse_list_ai	},
	{ "--ais",      parse_list_ai	},
	{ "--listais",	parse_list_ai	},
	{ "--listai",	parse_list_ai	},
	{ "--usage",    parse_usage	},
	{ "--help",     parse_usage	},
	{ "-h",    	parse_usage	},
	{ "-?",    	parse_usage	},
	{ "--test",	parse_test 	},

	{ "--debug-messages", parse_debug_messages },
};

/* The regular expression for matching URLS */
static const char *urlpattern = 
	"^tp"		/* The tp */
	"(\\+?http)?"    /* 1: Use Http */
	"(s)?" 		/* 2: ssh */
	"://"		

	"("		/* 3: Start optional pass/username */
	"([[:alnum:]]+)"	/* 4: Username */
	"(:"		/* 5: Start optional password */
	"([[:alnum:]]+)" /* 6: Password */
	")?"		
	"@)?"		/*   - End username */
	"([[:alnum:].]+)" /* 7: Server */
	"(:([[:digit:]]+))?" /* 9: Port */
	"/([[:alnum:]]+)?" /* 10: Game */
	"$";		/* Terminating '/' */

enum {
	REGEX_HTTP = 1,
	REGEX_SSH = 2,
	REGEX_USERNAME = 4,
	REGEX_PASSWORD = 6,
	REGEX_SERVER = 7,
	REGEX_PORT = 9,
	REGEX_GAME = 10,
};


#define MATCH(match,offset) 	(match[offset].rm_so != match[offset].rm_eo &&\
				match[offset].rm_so != -1)
#define EXTRACT(parent,str,match,offset) 	\
		talloc_strndup(parent, str + match[offset].rm_so,	\
				match[offset].rm_eo - match[offset].rm_so)

int
main(int argc, char **argv){
	struct tpe *tpe;
	struct startopt *opt;

	evas_init();
	ecore_init();

	tpe = talloc_zero(NULL,struct tpe);
	if (tpe == NULL) exit(1);

	opt = parse_args(argc, argv);
	if (opt == NULL){
		printf("Error parsing arguments\n");
		exit(1);
	}
	if (opt->showoptions)
		dump_options(opt);

	tpe->event 	= tpe_event_init();
	tpe->servers 	= server_init(tpe);
	//tpe->msg   	= tpe_msg_init(tpe);
	tpe_comm_init(tpe);
	
	tpe->sequence 	= tpe_sequence_init(tpe);

	tpe->obj   	= tpe_obj_init(tpe);
	tpe->orders  	= tpe_orders_init(tpe);
	tpe->resources 	= tpe_resources_init(tpe);
	tpe->board 	= tpe_board_init(tpe);
	tpe->ship	= tpe_ship_init(tpe);

//	if (opt->usegui)
//		tpe->gui = gui_init(tpe, opt->theme, opt->fullscreen);
	if (opt->ai && opt->ai->init)	
		tpe->ai = opt->ai->init(tpe);

	if (opt->browse)
		browser_add(tpe,opt->server);
	else if (opt->server && opt->username && opt->password)
		tpe_comm_connect(tpe, opt->server, opt->port, 
				opt->game, 
				opt->username, opt->password);

	ecore_main_loop_begin();

	return 0;
}


static struct startopt *
parse_args(int argc, char **argv){
	char *p;
	struct startopt *opt;
	int i,j;
	int rv;

	opt = talloc_zero(NULL, struct startopt);
	if (opt == NULL) return NULL;

	/* Set some defaults */
	p = strrchr(argv[0],'/');
	if (p == NULL)
		p = argv[0];
	else 
		p ++;
		
	/* Use defaults */
	opt->usegui = USE_GUI; /* Default */
	opt->ai = ai_info; /* FIXME: Compile constant */
	opt->username = strdup("nash");
	opt->password = strdup("password");
	opt->server = strdup("localhost");
	opt->game = NULL;
	opt->port = 6923;

	for (i = 1 ; i < argc ; i ++){
		for (j = 0 ; j < sizeof(args)/sizeof(args[0]) ; j ++){
			if (!strncmp(args[j].arg, argv[i],strlen(args[j].arg))){
				rv = args[j].fn(opt, i,argv);
				if (rv == -1){
					/* Failed */
					return opt;
				} else {
					rv += i;
				}
				break;
			}
		}
	}

	return opt;
}

static int
parse_usebrowser(struct startopt *opt, int i, char **args){
	opt->browse = 1;
	i ++;
	return i;
}

static int
parse_test(struct startopt *opt, int i, char **args){
	tpe_util_test();
	
	return i;
}

static int 
parse_username(struct startopt *opt, int i, char **args){
	const char *tmp;
	tmp = parse_option(args, &i);
	if (tmp){
		if (opt->username) free(opt->username);
		opt->username = strdup(tmp);
	}
	return i;
}

static int 
parse_password(struct startopt *opt, int i, char **args){
	const char *tmp;
	tmp = parse_option(args, &i);
	if (tmp)
		opt->password = strdup(tmp);
	return i;
}

static int 
parse_ai(struct startopt *opt, int i, char **args){
	const char *p;
	int j;

	p = parse_option(args, &i);
	if (p == NULL){
		printf("You must pass an AI\n");
		return i;
	}
	for (j = 0 ; j < N_AIS ; j ++){
		if (strcasecmp(ai_info[j].name,p) == 0){
			opt->ai = ai_info + j;
			break;
		}
	}
	if (j == N_AIS)
		printf("Unknown AI %s\n",p);

	return i;
}

static int 
parse_no_ai(struct startopt *opt, int i, char **args){
	opt->ai = NULL;
	return i;
}

static int
parse_list_ai(struct startopt *opt, int i, char **args){
	int j;

	printf("Known AIs are:\n");
	for (j = 0 ; j < N_AIS ; j ++){
		printf("%s:\n\t%s\n",ai_info[j].name,ai_info[j].description);
	}
	exit(0);
}

static int 
parse_gui(struct startopt *opt, int i, char **args){
	opt->usegui = 0;	
	return i;
}
static int 
parse_fullscreen(struct startopt *opt, int i, char **args){
	opt->fullscreen = 1;	
	return i;
}


static int 
parse_debug_messages(struct startopt *opt, int i, char **args){
	opt->debug.messages = 1;
	return i;
}

static int 
parse_theme(struct startopt *opt, int i, char **args){
	const char *tmp;
	tmp = parse_option(args, &i);
	if (tmp)
		opt->theme = strdup(tmp);
	return i;
}

static int 
parse_game(struct startopt *opt, int i, char **args){
	const char *tmp;
	tmp = parse_option(args, &i);
	if (tmp)
		opt->game = strdup(tmp);
	return i;

}


static int 
parse_server(struct startopt *opt, int i, char **args){
	const char *tmp;
	tmp = parse_option(args, &i);
	if (tmp)
		opt->server = strdup(tmp);
	return i;
}


static int 
parse_options(struct startopt *opt, int i, char **args){
	opt->showoptions = 1;
	return i;
}


static int 
parse_port(struct startopt *opt, int i, char **args){
	opt->port = strtol(parse_option(args,&i),0,0);
	return i;
}

static void
dump_options(struct startopt *opt){
	printf("tp%s%s://%s:%s@%s:%d/%s\n",
			opt->http ? "+http" : "",
			opt->ssl ? "s" : "",
			opt->username,
			opt->password, 
			opt->server,
			opt->port,
			opt->game ? opt->game : "");
	
	if (opt->ai) printf("Ai is %s\n",opt->ai->name);
	printf("Gui is %s\n",opt->usegui ? "On" : "Off");
	printf("Theme is %s\n",opt->theme);

	exit(0);

}

/**
 * Parse a TP formatted URL
 *
 * tp+?(http)s?://[username[:password]@]server[:port]/[game]
 *
 * Does not support games at this time
 *
 * Currently parses:
 * Scheme: 
 * 	tp
 * 
 */
static int 
parse_url(struct startopt *opt, int i, char **args){
	regex_t re;
	regmatch_t *matches;
	int rv;
	char *str;

	str = args[i];

	if ((rv = regcomp(&re, urlpattern, REG_EXTENDED)) != 0){
		tpe_regerror(rv,&re);
		return 1;
	}

	matches = calloc(re.re_nsub + 1, sizeof(regmatch_t));
	if (matches == NULL){
		perror("calloc");
		return 1;
	}

	rv = regexec(&re, str, re.re_nsub, matches, 0);
	if (rv != 0){
		tpe_regerror(rv,&re);
		return 1;
	}

	/* 1: Http */
	if (MATCH(matches,REGEX_HTTP)){
		opt->http = 1;
	}

	/* 2: ssh */
	if (MATCH(matches,REGEX_SSH)){
		opt->ssl = 1;
	}

	/* 3: Username */
	if (MATCH(matches,REGEX_USERNAME)){
		if (opt->username) free(opt->username);
		opt->username = EXTRACT(opt,str,matches,REGEX_USERNAME);
	}
	/* 4: Password */
	if (MATCH(matches,REGEX_PASSWORD)){
		if (opt->password) free(opt->password);
		opt->password = EXTRACT(opt,str,matches,REGEX_PASSWORD);
	}

	/* 5: Server */
	if (MATCH(matches,REGEX_SERVER)){
		if (opt->server) free(opt->server);
		opt->server = EXTRACT(opt,str,matches,REGEX_SERVER);
	}	

	/* 6: Port */
	if (MATCH(matches, REGEX_PORT)){
		opt->port = strtol(str + matches[REGEX_PORT].rm_so,0,10);
	}

	/* 10: Game name */
	if (MATCH(matches,REGEX_GAME)){
		if (opt->game) free(opt->game);
		opt->game = EXTRACT(opt,str,matches,REGEX_GAME);
	}

	free(matches);

	return i;
}

static int 
parse_usage(struct startopt *opt, int i, char **args){
	printf(
"Welcome to TPE : The Thousand Parsec Enlightened Client\n"
"\n"
"TPE features a basic AI as well as human user interface.\n"
"Future work includes using the AI for micro-management.\n"
"Please note this is a work in progress, and at this time\n"
"is not a complete client.\n"
"\n"
"Usage:\n"
"\ttpe [options] [server]\n"
"Where server is the full URL of the server in the form:\n"
"\tProtocol://[user[:password]@]server[:port]/\n"
"\t\tProtcol can only be tp at this time.\n"
"\t\tUser is the optional username.\n"
"\t\tPassword requires username to be present).\n"
"\t\tServer is the IP of name of the server - IPv4 or DNS.\n"
"\t\tPort (optional), else default port will be used.\n"
"Options:\n"
"\t--username,-u <User>       Log into server with this username.\n"
"\t--password,-p <Password>   Log into server with this password.\n"
"\t--server,-s <Server>       Server name (hostname or IP).\n"
"\t--ai <AI Name | None>      Use this AI, or none.\n"
"\t--list-ais		      List the AIs in this client.\n"
"\t--no-ai                    Turn off all AIs.\n"
"\t--no-gui                   Disable the GUI.\n"
"\t--theme,-t <Themename>     Name of edje theme file to use [GUI].\n"
"\t--fullscreen               Start in fullscreen mode [GUI].\n"
"\t--usage,--help,-h,-?       This help screen.\n"
"\t--options       	      Dump options and exit.\n"
);

	exit(0);
}

static const char *
parse_option(char **args, int *i){
	char *p;
	if ((p = strstr(args[*i], "="))){
		p ++;
	} else {
		if (args[*i + 1])
			p = args[*i + 1];
		(*i) ++;
	}

	return p;
}


/**
 * Error handler for regex expression parser.
 *
 * @param errcode Error code from regex library
 * @param regex The regular expression that caused the error
 * @return 0 
 */
static int
tpe_regerror(int errcode, const regex_t *regex){
	char buf[BUFSIZ];

	regerror(errcode,regex,buf,BUFSIZ);
	fprintf(stderr,"Regular expression error: %s\n",buf);
	return 0;
}
