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
#include "server.h"

struct bserver;

struct browser {
	struct bserver *servers;
};

struct bserver {
	struct browser *browser;
	struct bserver *next;
	struct tpe *tpe;
	struct server *server;
	char *name;
	char *ruleset;
	char *version;
	struct tpe_msg *msg;
};

static int browser_socket_connect(void *serverv, struct server *conn);
static int browser_connect_reply(void *serverv, struct msg *msg);


int
browser_add(struct tpe *tpe, const char *servername){
	struct browser *browser;
	struct bserver *bserver;

	browser = calloc(1,sizeof(struct browser));
	bserver = calloc(1,sizeof(struct bserver));
	bserver->name = strdup("localhost");
	bserver->next = browser->servers;
	bserver->browser = browser;
	browser->servers = bserver;

	bserver->server = server_connect(tpe, "localhost", 6923, 0, 
			browser_socket_connect, bserver);
	return 0;
}

static int
browser_socket_connect(void *bserverv, struct server *conn){
	struct bserver *bserver = bserverv; 

printf("Connect\n");
	server_send_strings(bserver->server, "MsgConnect", 
			browser_connect_reply, bserverv, 
			"GalaxiE", NULL);
	
	return 1;
}

static int 
browser_connect_reply(void *serverv, struct msg *msg){
	assert(msg); assert(serverv);
	assert(msg->len == 0 || msg->data != NULL);

	printf("Got a connect reply\n");
	return 0;
}

















