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

static struct features {
	int id;
	const char *desc;
} features[] =  {
	{ 1,	"SSL connection on this port" },
	{ 2,	"SSL connection on another port" },
	{ 3,	"http connect on this port" },
	{ 4,	"http connect on another port" },
	{ 5,	"keep alive" },
	{ 6,	"Serverside properties" },
	{ 1000,	"Automatic account registration" },
};
#define N_FEATURES (sizeof(features)/sizeof(features[0]))



struct tpe_comm {
	struct tpe *tpe;

	const char *server;
	int port;
	const char *user;
	const char *pass;


	struct connect *connect;
};

int tpe_comm_connect(struct tpe_comm *comm, const char *server, int port, 
			const char *user, const char *pass);
static int tpe_comm_socket_connect(void *data, struct tpe_msg_connection *);
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
			tpe_comm_available_features_msg, comm);
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
tpe_comm_connect(struct tpe_comm *comm, const char *server, int port, 
			const char *user, const char *pass){
	struct tpe *tpe;
	struct connect *connect;
	
	tpe = comm->tpe;
	assert(tpe);

	comm->server = strdup(server);
	comm->port = port;
	comm->user = strdup(user);
	comm->pass = strdup(pass);

	connect = calloc(1,sizeof(struct connect));
	connect->server = comm->server;
	connect->user = comm->user;
	connect->game = "Default";
	connect->status = CONSTATUS_CONNECTING;

	comm->connect = connect;

	tpe_msg_connect(tpe->msg, server, port, 0,
			tpe_comm_socket_connect, comm);

	tpe_event_send(tpe->event, "ConnectionStart", connect, 
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
				"EClient", NULL);
	return 1;
}

static int
tpe_comm_may_login(void *data, const char *msgtype, int len, void *mdata){
	struct tpe_comm *comm;
	struct tpe *tpe;
	struct tpe_msg *msg;

	comm = data;
	tpe = comm->tpe;
	msg = tpe->msg;


	tpe_msg_send_strings(msg, "MsgLogin",  tpe_comm_logged_in, comm,
			comm->user, comm->pass, 0);
	tpe_msg_send(msg, "MsgGetFeatures", 0,0,0,0);

	return 0;
}

static int
tpe_comm_logged_in(void *data, const char *msgtype, int len, void *mdata){
	struct tpe_comm *comm;
	struct tpe *tpe;
	struct tpe_msg *msg;
	int buf[3];

	/* FIXME: Need to check the access worked */
	comm = data;
	tpe = comm->tpe;
	msg = tpe->msg;

	tpe_msg_send(msg, "MsgGetTimeRemaining", 0, 0,0,0);

	/* FIXME: Need a object to handle these */
	buf[0] = htonl(1);	/* One player to get */
	buf[1] = htonl(0); 	/* 0 = Myself */
	tpe_msg_send(msg, "MsgGetPlayerData", tpe_comm_msg_player_id, tpe, 
			buf, 8);

	ecore_timer_add(5, tpe_comm_get_time, msg);

	tpe_event_send(tpe->event, "Connected", comm->connected, 
			tpe_event_nofree, NULL);

	return 0;
}

static int
tpe_comm_get_time(void *msg){
	tpe_msg_send(msg, "MsgGetTimeRemaining", 0,0,0,0);
	return 1;
}

static int 
tpe_comm_msg_player_id(void *userdata, const char *msgtype,
                int len, void *edata){
	struct tpe *tpe = userdata;
	
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
		for (j = 0 ; j < N_FEATURES ; j ++)
			if (features[j].id == feature)
				printf("\tFeature: %s\n",features[j].desc);
	}

	return 1;

}

static int
tpe_comm_msg_fail(void *udata, int etype, void *event){
	int magic, type, seq, len, errcode;
	int rv;
	char *str = 0;

	rv = tpe_util_parse_packet(event, "iiiiis", &magic, &seq, &type, &len,
			&errcode, &str);

	printf("** Error: Seq %d: Err %d: %s\n",seq,errcode,str);

	exit(1);
	
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
