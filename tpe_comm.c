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
static int tpe_comm_msg_fail(void *udata, int type, void *event);
static int tpe_comm_time_remaining(void *udata, int type, void *event);

static int tpe_comm_msg_player_id(void *userdata, struct msg *msg);

static int tpe_comm_get_time(void *msg);

/* Generic handlers */
static int tpe_comm_available_features_msg(void *udata, int type, void *event);



void
tpe_comm_init(struct tpe *tpe){
	/* FIXME: Need to check for multi start here */
	tpe_event_handler_add(tpe->event, "MsgAvailableFeatures", 
			tpe_comm_available_features_msg, tpe);
	tpe_event_handler_add(tpe->event, "MsgFail",
			tpe_comm_msg_fail, tpe);
	tpe_event_handler_add(tpe->event, "MsgTimeRemaining",
			tpe_comm_time_remaining, tpe);

	tpe_event_type_add(tpe->event, "NewTurn");
	tpe_event_type_add(tpe->event, "ConnectStart");
	tpe_event_type_add(tpe->event, "ConnectUpdate");
	tpe_event_type_add(tpe->event, "Connected");
	tpe_event_type_add(tpe->event, "HaveServerFeatures");
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

	tpe_event_send(tpe->event, "ConnectStart", connect, 
			tpe_event_nofree, NULL);

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
#if 0
	buf[0] = htonl(1);	/* One player to get */
	buf[1] = htonl(0); 	/* 0 = Myself */
	server_send_format(msg->server,"MsgGetPlayerData",
			tpe_comm_msg_player_id,"i0",1);
#endif

	ecore_timer_add(5, tpe_comm_get_time, msg->server);
	/* The rest of the system know */
	tpe_event_send(msg->tpe->event, "Connected", connect, 
			tpe_event_nofree, NULL);

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


static int
tpe_comm_get_time(void *server){
	server_send(server, "MsgGetTimeRemaining", NULL,NULL,NULL,0);
	return 1;
}

static int 
tpe_comm_msg_player_id(void *userdata, struct msg *msg){
	struct tpe *tpe = userdata;

	if (strcmp(msg->type, "MsgFail") == 0){
		/* Message fail! */
		/* FIXME: Need to trigger an 'Error' message */
		assert(!"MsgFail");
		exit(1);
	}

	tpe_util_parse_packet(msg->data, msg->end, "iss", &tpe->player,
			&tpe->racename, &tpe->playername);

	return 1;
}



/**
 * Event Handler for "Available features" message.
 *
 * Currently jsut prints out the data
 */
static int 
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

static int
tpe_comm_msg_fail(void *udata, int etype, void *event){
	int magic, type, seq, len, errcode;
	int rv;
	char *str = NULL;

assert(!"Incorrect callback!\n");
	rv = tpe_util_parse_packet(event, NULL,
			"His", &magic, &seq, &type, &len,
			&errcode, &str);

	printf("** Error: Seq %d: Err %d: %s\n",seq,errcode,str);

	free(str);

	return 1;
}




static int 
tpe_comm_time_remaining(void *udata, int type, void *event){
	struct msg *msg = event;
	struct tpe *tpe;
	int remain,rv;

	tpe = udata;

	rv = tpe_util_parse_packet(msg->data, msg->end, "i", &remain);
	if (rv != 1) {
		printf("Failed to parse time remaining packet\n");
		return 0;
	}
	
	if (msg->seq == 0 && remain == 0){
		tpe->turn ++;
		tpe_event_send(tpe->event, "NewTurn", msg->server, NULL, NULL);
	}

	return 1;
}


static void 
feature_handler_accountregister(struct tpe *tpe, int id, struct features *feat){
	//struct tpe_comm *comm;
	/* FIXME: Implement this function */
	if (!tpe || !id || !feat) return;

	//comm->accountregister = 1;
}
