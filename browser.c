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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "browser.h"
#include "server.h"
#include "tpe_util.h"

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
static int browser_games_receive(void *serverv, struct msg *msg);


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

	server_send_strings(bserver->server, "MsgConnect", 
			browser_connect_reply, bserverv, 
			"GalaxiE", NULL);
	
	return 1;
}

static int 
browser_connect_reply(void *serverv, struct msg *msg){

	SERVER_MSG_VALID(msg);
	assert(serverv);

	printf("Got a connect reply\n");
	/* Get games frame */
	server_send(msg->server, "MsgGetGames",
			browser_games_receive, serverv,
			NULL, 0);
	printf("sending!\n");
	return 0;
}

static int
browser_games_receive(void *serverv, struct msg *msg){
	const char *name = NULL, *key = NULL;

	SERVER_MSG_VALID(msg);

	printf("Received game info\n");

	/* Note: This thing is a monster */
	tpe_util_parse_packet(msg->data, msg->end, 
			"ss", &name, &key);
		
	printf("Game: %s %s\n",name,key);

	return 1;
}

















