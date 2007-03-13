#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Con.h>
#include <Edje.h>

#include "tpe.h"
#include "tpe_board.h"
#include "tpe_comm.h"
#include "tpe_event.h"
#include "tpe_gui.h"
#include "tpe_msg.h"
#include "tpe_obj.h"
#include "tpe_orders.h"
#include "tpe_resources.h"
#include "tpe_sequence.h"
#include "tpe_ship.h"

#include "ai_smith.h"

enum opt_ai {
	AI_NONE,
	AI_SMITH,
};


struct startopt {
	const char *username;
	const char *password;
	const char *server;
	
	/* AI options */
	enum opt_ai ai;

	/* GUI options */
	int usegui;
	const char *theme;
};

static struct startopt *parse_args(int argc, char **argv);
static int parse_username(struct startopt *opt, int i, char **args);
static int parse_password(struct startopt *opt, int i, char **args);
static int parse_server(struct startopt *opt, int i, char **args);
static int parse_ai(struct startopt *opt, int i, char **args);
static int parse_no_ai(struct startopt *opt, int i, char **args);
static int parse_gui(struct startopt *opt, int i, char **args);
static int parse_theme(struct startopt *opt, int i, char **args);
static int parse_usage(struct startopt *opt, int i, char **args);
static const char *parse_option(char **args, int *i);

struct args {
	const char *arg;
	int (*fn)(struct startopt *opt, int i, char **args);
} args[] = {
	{ "--user",	parse_username },
	{ "-u",		parse_username },
	{ "--password", parse_password },
	{ "-p",		parse_password },
	{ "--server",	parse_server   },
	{ "-s",		parse_server   },
	{ "--ai",	parse_ai       },
	{ "--no-ai",	parse_no_ai    },
	{ "--no-gui",   parse_gui      },
	{ "--theme",    parse_theme    },
	{ "-t",         parse_theme    },
	{ "--usage",    parse_usage    },
	{ "--help",     parse_usage    },
	{ "-h",    	parse_usage    },
	{ "-?",    	parse_usage    },
};


int
main(int argc, char **argv){
	struct tpe *tpe;
	struct startopt *opt;

	evas_init();
	ecore_init();

	tpe = calloc(1,sizeof(struct tpe));
	if (tpe == NULL) exit(1);

	opt = parse_args(argc, argv);

	tpe->event 	= tpe_event_init(tpe);
	tpe->msg   	= tpe_msg_init(tpe);
	tpe->comm  	= tpe_comm_init(tpe);
	
	tpe->sequence 	= tpe_sequence_init(tpe);

	tpe->obj   	= tpe_obj_init(tpe);
	tpe->orders  	= tpe_orders_init(tpe);
	tpe->resources 	= tpe_resources_init(tpe);
	tpe->board 	= tpe_board_init(tpe);
	tpe->ship	= tpe_ship_init(tpe);

	if (opt->usegui)
		tpe->gui	= tpe_gui_init(tpe);
	if (opt->ai == AI_SMITH)
		tpe->ai 	= ai_smith_init(tpe);

	if (opt->server && opt->username && opt->server)
		tpe_comm_connect(tpe->comm, opt->server, 6923, opt->username,
			opt->password);

	ecore_main_loop_begin();

	return 0;
}


static struct startopt *
parse_args(int argc, char **argv){
	struct startopt *opt;
	int i,j;

	opt = calloc(1,sizeof(struct startopt));
	/* Set some defaults */
	opt->usegui = 1; /* Default */ /* FIXME: Compile constant */
	opt->ai = AI_SMITH; /* FIXME: Compile constant */
	opt->username = "nash";
	opt->password = "password";
	opt->server = "localhost";

	for (i = 1 ; i < argc ; i ++){
		for (j = 0 ; j < sizeof(args)/sizeof(args[0]) ; j ++)	
			if (!strncmp(args[j].arg, argv[i],strlen(args[j].arg)))
				i = args[j].fn(opt, i,argv);
	}

	return opt;
}



int 
parse_username(struct startopt *opt, int i, char **args){
	opt->username = parse_option(args,&i);
	return i;
}

int 
parse_password(struct startopt *opt, int i, char **args){
	opt->password = parse_option(args,&i);
	return i;
}

int 
parse_ai(struct startopt *opt, int i, char **args){
	const char *p;

	p = parse_option(args, &i);
	if (strcasecmp(p, "none") == 0)
		opt->ai = AI_NONE;
	if (strcasecmp(p, "smith") == 0)
		opt->ai = AI_SMITH;
	else
		printf("Unknown AI %s\n",p);

	return i;
}

static int 
parse_no_ai(struct startopt *opt, int i, char **args){
	opt->ai = AI_NONE;
	return i;
}

int 
parse_gui(struct startopt *opt, int i, char **args){
	opt->usegui = 0;	
	return i;
}

int 
parse_theme(struct startopt *opt, int i, char **args){
	opt->theme = parse_option(args, &i);
	
	return i;	
}


static int 
parse_server(struct startopt *opt, int i, char **args){
	opt->server = parse_option(args, &i);
	return i;
}

static int 
parse_usage(struct startopt *opt, int i, char **args){
	printf("This is the help. \n");
	printf("It will be implemented soon\n");
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



