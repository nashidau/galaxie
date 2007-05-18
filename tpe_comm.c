/*
 * TPE Comm - high level communications with the server
 */
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore.h>

/* TPE Comm can only call tpe, tpe_msg, tpe_event */
#include "tpe.h"
#include "tpe_comm.h"
#include "tpe_msg.h"
#include "tpe_event.h"
#include "tpe_util.h"


struct features {
	int id;
	const char *desc;
	void (*handler)(struct tpe *tpe, int id, struct features *);
};

static void feature_handler_autoaccount(struct tpe *tpe, int id, 
		struct features *);

static struct features features[] =  {
	{ 1,	"SSL connection on this port",		NULL },
	{ 2,	"SSL connection on another port",	NULL },
	{ 3,	"http connect on this port",		NULL },
	{ 4,	"http connect on another port",		NULL },
	{ 5,	"keep alive",				NULL },
	{ 6,	"Serverside properties",		NULL },
	{ 1000,	"Automatic account registration", feature_handler_autoaccount },
};
#define N_FEATURES (sizeof(features)/sizeof(features[0]))



struct tpe_comm {
	struct tpe *tpe;

	const char *server;
	int port;
	const char *user;
	const char *pass;
	const char *game;

	struct connect *connect;

	unsigned int hasautoaccount	: 1;
	unsigned int triedcreate	: 1;
};

static int tpe_comm_socket_connect(void *data, struct tpe_msg_connection *);
static void tpe_comm_create_account(struct tpe *tpe);
static int tpe_comm_create_account_cb(void *tpev, const char *msgtype, int len, void*data);
static int tpe_comm_may_login(void *data, const char *msgtype, int len, void *mdata);
static int tpe_comm_logged_in(void *data, const char *msgtype, int len, void *mdata);
static int tpe_comm_msg_fail(void *udata, int type, void *event);
static int tpe_comm_time_remaining(void *udata, int type, void *event);

static int tpe_comm_msg_player_id(void *userdata, const char *msgtype,
                int len, void *edata);

static int tpe_comm_get_time(void *msg);

/* Generic handlers */
static int tpe_comm_available_features_msg(void *udata, int type, void *event);



struct tpe_comm *
tpe_comm_init(struct tpe *tpe){
	struct tpe_comm *comm;

	assert(tpe->comm == NULL);

	comm = calloc(1,sizeof(struct tpe_comm));
	comm->tpe = tpe;

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


	return comm;
}


int
tpe_comm_connect(struct tpe_comm *comm, 
			const char *server, int port, 
			const char *game,
			const char *user, const char *pass){
	struct tpe *tpe;
	struct connect *connect;
	
	tpe = comm->tpe;
	assert(tpe);

	comm->server = strdup(server);
	comm->port = port;
	comm->user = strdup(user);
	comm->pass = strdup(pass);
	comm->game = strdup(game);

	connect = calloc(1,sizeof(struct connect));
	connect->server = comm->server;
	connect->user = comm->user;
	connect->game = game;
	connect->status = CONSTATUS_CONNECTING;

	comm->connect = connect;

	tpe_msg_connect(tpe->msg, server, port, 0,
			tpe_comm_socket_connect, comm);

	tpe_event_send(tpe->event, "ConnectStart", connect, 
			tpe_event_nofree, NULL);

	return 1;
}


static int
tpe_comm_socket_connect(void *data, struct tpe_msg_connection *mcon){
	struct tpe_comm *comm;
	struct tpe *tpe;
	struct tpe_msg *msg;

	comm = data;
	tpe = comm->tpe;
	msg = tpe->msg;

	tpe_msg_send_strings(msg,"MsgConnect", tpe_comm_may_login, comm,
				"GalaxiE", NULL);
	return 1;
}

static int
tpe_comm_may_login(void *data, const char *msgtype, int len, void *mdata){
	struct tpe_comm *comm;
	struct tpe *tpe;
	struct tpe_msg *msg;
	char buf[100];

	comm = data;
	tpe = comm->tpe;
	msg = tpe->msg;

	if (comm->game){
		snprintf(buf, 100, "%s@%s", comm->user, comm->game);
	} else {
		snprintf(buf, 100, "%s", comm->user);
	}

	tpe_msg_send_strings(msg, "MsgLogin",  tpe_comm_logged_in, comm,
			buf, comm->pass, 0);
	tpe_msg_send(msg, "MsgGetFeatures", NULL,NULL,NULL,0);

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
tpe_comm_logged_in(void *data, const char *msgtype, int len, void *mdata){
	struct tpe_comm *comm;
	struct tpe *tpe;
	struct tpe_msg *msg;
	int buf[3];

	assert(data); assert(msgtype); 

	comm = data;
	assert(comm->tpe);
	tpe = comm->tpe;
	assert(tpe->msg);
	msg = tpe->msg;

	/* FIXME: Need to check the access worked */
	if (!msgtype || strcmp(msgtype,"MsgFail") == 0){
		printf("Could not log in using the user name listed\n");
		/* FIXME: Need general error handling */
		if (comm->triedcreate == 0 && !comm->hasautoaccount)
			tpe_comm_create_account(tpe);
		else
			exit(1);
	}
	


	tpe_msg_send(msg, "MsgGetTimeRemaining", NULL, NULL,NULL,0);

	/* FIXME: Need a class to handle these */
	buf[0] = htonl(1);	/* One player to get */
	buf[1] = htonl(0); 	/* 0 = Myself */
	tpe_msg_send(msg, "MsgGetPlayerData", tpe_comm_msg_player_id, tpe, 
			buf, 8);

	ecore_timer_add(5, tpe_comm_get_time, msg);

	/* The rest of the system know */
	tpe_event_send(tpe->event, "Connected", comm->connect, 
			tpe_event_nofree, NULL);

	return 0;
}

/**
 * Try to create an account - on success, will cause the system to login
 * by default
 *
 */
static void
tpe_comm_create_account(struct tpe *tpe){
	struct tpe_comm *comm;

	assert(tpe);
	assert(tpe->comm);
	assert(tpe->comm->user); assert(tpe->comm->pass);

	comm = tpe->comm;

	tpe_msg_send_format(tpe->msg, "MsgCreateAccount", 
			tpe_comm_create_account_cb, tpe,
			"ssss", 
			comm->user,
			comm->pass,
			"",
			"Another satisfied GalaxiE user.");
	comm->triedcreate = 1;

}

static int
tpe_comm_create_account_cb(void *tpev, const char *msgtype, int len, void*data){
	struct tpe *tpe = tpev;
	struct tpe_comm *comm;
	char buf[100];

	assert(tpe != NULL); assert(msgtype);
	assert(tpe->comm);

	comm = tpe->comm;

	if (strcmp(msgtype, "MsgFail") == 0){
		printf("Could not create account\n");
		return 0;
	}

	if (comm->game){
		snprintf(buf,100,"%s@%s", comm->user, comm->game);
	} else {
		strncpy(buf,comm->user,100);
	}

	tpe_msg_send_strings(tpe->msg, "MsgLogin",  tpe_comm_logged_in, comm,
			buf, comm->pass, 0);

	return 0;

}


static int
tpe_comm_get_time(void *msg){
	tpe_msg_send(msg, "MsgGetTimeRemaining", NULL,NULL,NULL,0);
	return 1;
}

static int 
tpe_comm_msg_player_id(void *userdata, const char *msgtype,
                int len, void *edata){
	struct tpe *tpe = userdata;

	if (msgtype != NULL && strcmp(msgtype, "MsgFail") == 0){
		/* Message fail! */
		/* FIXME: Need to trigger an 'Error' message */
		assert(!"MsgFail");
		exit(1);
	}

	tpe_util_parse_packet(edata, "iss", &tpe->player,
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
	struct tpe *tpe = udata;
	uint32_t *data;
	int feature;
	uint32_t i,j,len;

	data = (uint32_t *)event + 4;
	len = ntohl(*data);
	if (len > 32) len = 32;

	if ((len + 1) * 4!= ntohl(data[-1]))
		printf("Data lengths don't match: %d items vs %d bytes\n",
				len, ntohl(data[-1]));

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

	rv = tpe_util_parse_packet(event, "iiiiis", &magic, &seq, &type, &len,
			&errcode, &str);

	printf("** Error: Seq %d: Err %d: %s\n",seq,errcode,str);

	return 1;
}




static int 
tpe_comm_time_remaining(void *udata, int type, void *event){
	struct tpe *tpe;
	struct tpe_msg *msg;
	int magic,etype,seq;
	int unused, remain;

	tpe = udata;
	msg = tpe->msg;

	tpe_util_parse_packet(event, "iiiii",&magic, &seq, &etype, &unused, 
			&remain);

	if (seq == 0 && remain == 0){
		tpe->turn ++;
		tpe_event_send(tpe->event, "NewTurn", strdup("pants"), NULL, NULL);
	}

	return 1;
}


static void 
feature_handler_autoaccount(struct tpe *tpe, int id, struct features *feat){
	struct tpe_comm *comm;

	assert(tpe); assert(id); assert(feat); assert(tpe->comm);

	if (!tpe || !id || !feat) return;
	if (!tpe->comm) return;

	comm = tpe->comm;

	comm->hasautoaccount = 1;
}
