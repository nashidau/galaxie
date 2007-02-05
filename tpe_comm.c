/*
 * TPE Comm - high level communications with the server
 */
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TPE Comm can only call tpe, tpe_msg, tpe_event */
#include "tpe.h"
#include "tpe_msg.h"
#include "tpe_event.h"


struct tpe_comm {
	struct tpe *tpe;

	const char *server;
	int port;
	const char *user;
	const char *pass;
};

int tpe_comm_connect(struct tpe_comm *comm, const char *server, int port, 
			const char *user, const char *pass);
static int tpe_comm_socket_connect(void *data, struct tpe_msg_connection *);
static int tpe_comm_may_login(void *data, const char *msgtype, int len, void *mdata);
static int tpe_comm_logged_in(void *data, const char *msgtype, int len, void *mdata);

struct tpe_comm *
tpe_comm_init(struct tpe *tpe){
	struct tpe_comm *comm;

	comm = calloc(1,sizeof(struct tpe_comm));
	comm->tpe = tpe;

	return comm;
}


int
tpe_comm_connect(struct tpe_comm *comm, const char *server, int port, 
			const char *user, const char *pass){
	struct tpe *tpe;
	
	tpe = comm->tpe;
	assert(tpe);

	comm->server = strdup(server);
	comm->port = port;
	comm->user = strdup(user);
	comm->pass = strdup(pass);

	tpe_msg_connect(tpe->msg, server, port, 0,
			tpe_comm_socket_connect, comm);

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

	printf("We have a socket... now to do something with it\n");

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
			"nash", "pass", 0);
	tpe_msg_send(msg, "MsgGetFeatures", 0,0,0,0);

	return 0;
}

static int
tpe_comm_logged_in(void *data, const char *msgtype, int len, void *mdata){
	struct tpe_comm *comm;
	struct tpe *tpe;
	struct tpe_msg *msg;
	int buf[3];

	comm = data;
	tpe = comm->tpe;
	msg = tpe->msg;
printf("Sending lots of stuff\n");
	tpe_msg_send(msg, "MsgGetTimeRemaining", 0, 0,0,0);
	
	/* Fire of some sequences */
	buf[0] = htonl(-1);	/* New seq */
	buf[1] = htonl(0);	/* From 0 */
	buf[2] = htonl(-1);	/* Get them all */

	tpe_msg_send(msg, "MsgGetBoardIDs", 0,0,buf,12);
	tpe_msg_send(msg, "MsgGetResourceIDs", 0,0,buf,12);
	tpe_msg_send(msg, "MsgGetObjectIDs", 0,0,buf,12);

	return 0;
}



