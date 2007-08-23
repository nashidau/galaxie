/**
 * Browser:
 * 	Attempts to find games on the network.
 * 	Uses:
 * 		- Provided server on command line [Not implemented]
 * 		- Localhost [Partially implemented]
 * 		- Avahi [Not implemented]
 * 		- Metaserver [Not implemented]
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "tpe_msg.h"

struct server;

struct browser {
	struct server *servers;
};

struct server {
	struct browser *browser;
	struct server *next;
	struct tpe *tpe;
	struct tpe_msg_connection *conn;
	char *name;
	char *ruleset;
	char *version;
	struct tpe_msg *msg;
};

static int browser_socket_connect(void *serverv, struct tpe_msg_connection *conn);
static int browser_connect_reply(void *serverv, const char *msgtype, int len, void *data);


int
browser_add(struct tpe *tpe, const char *servername){
	struct browser *browser;
	struct server *server;

	browser = calloc(1,sizeof(struct browser));
printf("Browser add\n");
	server = calloc(1,sizeof(struct server));
	server->name = strdup("localhost");
	server->next = browser->servers;
	server->browser = browser;
	browser->servers = server;

	server->msg = tpe_msg_connect(tpe, "localhost", 6923, 0, 
			browser_socket_connect, server);
	return 0;
}

static int
browser_socket_connect(void *serverv, struct tpe_msg_connection *conn){
	struct server *server = serverv; 

	server->conn = conn;
printf("Connect\n");
	tpe_msg_send_strings(server->msg, "MsgConnect", 
			browser_connect_reply, serverv, 
			"GalaxiE", NULL);
	
	return 1;
}

static int 
browser_connect_reply(void *serverv, const char *msgtype, int len, void *data){
	assert(msgtype); assert(serverv); assert(len == 0 || data);

	printf("Got a connect reply\n");
	return 0;
}

















