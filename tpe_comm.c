/*
 * TPE Comm - high level communications with the server
 */
#include <arpa/inet.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore.h>

/* TPE Comm can only call tpe, tpe_msg, tpe_event */
#include "tpe.h"
#include "tpe_comm.h"
#include "server.h"
#include "tpe_event.h"
#include "tpe_util.h"


struct features {
	int id;
	const char *desc;
	void (*handler)(struct tpe *tpe, int id, struct features *);
};

static void feature_handler_accountregister(struct tpe *tpe, int id, 
		struct features *);

static struct features features[] =  {
	{ 1,	  "SSL connection on this port",		NULL },
	{ 2,	  "SSL connection on another port",	NULL },
	{ 3,	  "http connect on this port",		NULL },
	{ 4,	  "http connect on another port",		NULL },
	{ 5,	  "keep alive",				NULL },
	{ 6,	  "Serverside properties",		NULL },
	{ 0x3e8,  "Account Creation Allowed",		NULL },
	{ 0x1000, "SSL filer can enbabled",		NULL },
	{ 0x1D00, "Padding may be enabled",		NULL },
	{ 0x10000,"Object ID in descending modified time", NULL },
	{ 0x10001,"Order ID in descending modified time", NULL },
	{ 0x10002,"Board ID in descending modified time", NULL },
	{ 0x10003,"Resource ID in descending modified time", NULL },
	{ 0x10004,"Category ID in descending modified time", NULL },
	{ 0x10005,"Design ID in descending modified time", NULL },
	{ 0x10006,"Component ID in descending modified time", NULL },
	{ 0x10007,"Property ID in descending modified time", NULL },
	{ 1000,	"Account registration", feature_handler_accountregister },
};
#define N_FEATURES (sizeof(features)/sizeof(features[0]))



struct tpe_comm {
	struct tpe *tpe;

	struct server *server;

	const char *servername;
	int port;
	const char *user;
	const char *pass;
	const char *game;

	struct connect *connect;
	
	/* Have downloaded 'features' */
	unsigned int features		: 1;
	unsigned int accountregister	: 1;
	unsigned int triedcreate	: 1;
};

static int tpe_comm_socket_connect(void *data, struct server *);
static void tpe_comm_create_account(struct connect *connect);
static int tpe_comm_create_account_cb(void *tpev, struct msg *msg);
static int tpe_comm_may_login(void *data, struct msg *msg);
static int tpe_comm_logged_in(void *data, struct msg *msg);
static Eina_Bool tpe_comm_msg_fail(void *udata, int type, void *event);
static Eina_Bool tpe_comm_time_remaining(void *udata, int type, void *event);

static int tpe_comm_msg_player_id(void *userdata, struct msg *msg);

static Eina_Bool tpe_comm_get_time(void *msg);

/* Generic handlers */
static Eina_Bool tpe_comm_available_features_msg(void *udata, int type, void *);



void
tpe_comm_init(struct tpe *tpe){
	/* FIXME: Need to check for multi start here */
	tpe_event_handler_add("MsgAvailableFeatures",
			tpe_comm_available_features_msg, tpe);
	tpe_event_handler_add("MsgFail", tpe_comm_msg_fail, tpe);
	tpe_event_handler_add("MsgTimeRemaining",tpe_comm_time_remaining, tpe);

	tpe_event_type_add("NewTurn");
	tpe_event_type_add("ConnectStart");
	tpe_event_type_add("ConnectUpdate");
	tpe_event_type_add("Connected");
	tpe_event_type_add("HaveServerFeatures");
}


int
tpe_comm_connect(struct tpe *tpe,
			const char *server, int port,
			const char *game,
			const char *user, const char *pass){
	struct connect *connect;

	connect = calloc(1,sizeof(struct connect));
	connect->servername = strdup(server);
	connect->user = strdup(user);
	connect->pass = strdup(pass);
	if (game)
		connect->game = strdup(game);
	else
		connect->game = NULL;
	connect->status = CONSTATUS_CONNECTING;

	connect->server = server_connect(tpe, server, port, 0,
			tpe_comm_socket_connect, connect);

	tpe_event_send("ConnectStart", connect, tpe_event_nofree, NULL);

	return 1;
}


static int
tpe_comm_socket_connect(void *data, struct server *server){
	struct connect *connect;

	connect = data;

	assert(connect->server == server);

	server_send_strings(connect->server,"MsgConnect", 
				tpe_comm_may_login, connect,
				"GalaxiE", NULL);
	return 1;
}

static int
tpe_comm_may_login(void *data, struct msg *msg){
	struct connect *connect;
	char buf[100];

	connect = data;
	//tpe = connect->tpe;

	if (connect->game){
		snprintf(buf, 100, "%s@%s", connect->user, connect->game);
	} else {
		snprintf(buf, 100, "%s", connect->user);
	}

	server_send(msg->server, "MsgGetFeatures", NULL,NULL,NULL,0);
	server_send_strings(msg->server, "MsgLogin",
			tpe_comm_logged_in, connect,
			buf, connect->pass, 0);

	return 0;
}

/**
 * Callback for 'logged in' - I hope.
 *
 * If autoconnect is set, and there is a failure, it will try once to register
 * the account first.
 *
 */
static int
tpe_comm_logged_in(void *data, struct msg *msg){
	struct connect *connect;

	assert(data); assert(msg); 

	connect = data;

	/* FIXME: Need to check the access worked */
	if (strcmp(msg->type,"MsgFail") == 0){
		if (connect->triedcreate == 0 && connect->accountregister){
			tpe_comm_create_account(connect);
			return 0;
		} else {
			printf("FIXME: Failed to connect \n");
			exit(1);
		}
	}
	


	server_send(msg->server, "MsgGetTimeRemaining", NULL, NULL,NULL,0);

	/* FIXME: Need a class to handle these */
	server_send_format(msg->server,"MsgGetPlayerData",
			tpe_comm_msg_player_id,msg->tpe,"i0",1);

	ecore_timer_add(5, tpe_comm_get_time, msg->server);
	/* The rest of the system know */
	tpe_event_send("Connected", connect, tpe_event_nofree, NULL);

	return 0;
}

/**
 * Try to create an account - on success, will cause the system to login
 * by default
 *
 */
static void
tpe_comm_create_account(struct connect *connect){
	server_send_format(connect->server, "MsgCreateAccount", 
			tpe_comm_create_account_cb, connect,
			"ssss", 
			connect->user,
			connect->pass,
			"",
			"Another satisfied GalaxiE user.");
	connect->triedcreate = 1;

}

static int
tpe_comm_create_account_cb(void *connectv, struct msg *msg){
	struct connect *connect = connectv;
	char buf[100];

	if (strcmp(msg->type, "MsgFail") == 0){
		printf("Could not create account\n");
		return 0;
	}

	if (connect->game){
		snprintf(buf,100,"%s@%s", connect->user, connect->game);
	} else {
		snprintf(buf, 100, "%s", connect->user);
	}

	server_send_strings(msg->server, "MsgLogin", 
			tpe_comm_logged_in, connect,
			buf, connect->pass, NULL);

	return 0;
}


static Eina_Bool
tpe_comm_get_time(void *server){
	server_send(server, "MsgGetTimeRemaining", NULL,NULL,NULL,0);
	return 1;
}

/**
 * FIXME: THis is entirely broken now.
 *
 * This used to a single number, now it's a sequence.
 * So this needs to be updated to reflect that.
 * Probably best to use the standard sequence handling routines if I can.
 */
static int
tpe_comm_msg_player_id(void *userdata, struct msg *msg){
	struct tpe *tpe = userdata;
	uint64_t modtime;
	int i;

	if (strcmp(msg->type, "MsgFail") == 0){
		/* Message fail! */
		/* FIXME: Need to trigger an 'Error' message */
		assert(!"MsgFail");
		exit(1);
	}

	if (strcmp(msg->type, "MsgSEQUENCE") == 0){
		/* Ahh.. just the sequence header... drop it */
		return 1;
	}

for (i = 0 ; i < msg->len / 4; i ++)
printf("%08x ",((uint32_t *)msg->data)[msg->len / 4]);
printf("\n");

	tpe_util_parse_packet(msg->data, msg->end, "issl", &tpe->player,
			&tpe->racename, &tpe->playername, &modtime);
	printf("Player %d: %s (%s)\n",tpe->player, tpe->racename,
			tpe->playername);

	return 1;
}



/**
 * Event Handler for "Available features" message.
 *
 * Currently just prints out the data
 */
static Eina_Bool
tpe_comm_available_features_msg(void *udata, int type, void *event){
	struct msg *msg;
	struct tpe *tpe = udata;
	uint32_t *data;
	int feature;
	uint32_t i,j,len;

	msg = event;
	data = msg->data;

	len = ntohl(*(int*)data);

	if ((len + 1) * 4!= msg->len)
		printf("Data lengths don't match: %d items vs %d bytes\n",
				len, msg->len);

	for (i = 0 ; i < len ; i ++){
		feature = ntohl(data[i + 1]);
		for (j = 0 ; j < N_FEATURES ; j ++){
			if (features[j].id == feature){
				printf("\tFeature: %s\n",features[j].desc);
				if (features[j].handler){
					features[j].handler(tpe,i,features + j);
				}
			}
		}
	}

	return 1;

}

static Eina_Bool
tpe_comm_msg_fail(void *udata, int etype, void *event){
	struct msg *msg = event;
	int rv;
	int errcode;
	char *str = NULL;

	rv = tpe_util_parse_packet(msg->data, msg->end, "is", &errcode, &str);

	printf("** Error: Seq %d: Err %d: %s\n",msg->seq,errcode,str);

	free(str);

	return 1;
}




static Eina_Bool
tpe_comm_time_remaining(void *udata, int type, void *event){
	struct msg *msg = event;
	struct tpe *tpe;
	int remain,rv;
	int turn;
	const char *turnname;

	tpe = udata;

	/* FIXME: Only get the turn name when the turn != tpe->turn */
	rv = tpe_util_parse_packet(msg->data, msg->end, "iis", &remain,
			&turn, &turnname);
	if (rv < 1) {
		printf("Failed to parse time remaining packet\n");
		return 0;
	}

	if (rv > 1 && tpe->turn != turn){
		/* Turn just ticked over */
		tpe->turn = turn;
	}

	if (msg->seq == 0 && remain == 0){
		tpe->turn ++;
		tpe_event_send("NewTurn", msg->server, tpe_event_nofree, NULL);
	}
	printf("Time: %4d\tTurn %2d (%s)\n", remain,turn, turnname);

	free((char *)turnname);
	return 1;
}


static void 
feature_handler_accountregister(struct tpe *tpe, int id, struct features *feat){
	//struct tpe_comm *comm;
	/* FIXME: Implement this function */
	if (!tpe || !id || !feat) return;

	//comm->accountregister = 1;
}
