/*
 * TPE Comm - high level communications with the server
 */

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
	printf("Connected\n");
	return 1;
}




