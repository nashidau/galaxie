/* 
 * General object related functions.
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "tpe.h"
#include "tpe_obj.h"
#include "tpe_event.h"
#include "tpe_msg.h"

struct tpe_obj {
	struct tpe *tpe;
};

static int tpe_obj_object_list(void *data, int eventid, void *event);

struct tpe_obj *
tpe_obj_init(struct tpe *tpe){
	struct tpe_event *event;
	struct tpe_obj *obj;

	event = tpe->event;

	obj = calloc(1,sizeof(struct tpe_obj));
	obj->tpe = tpe;

	tpe_event_handler_add(event, "MsgListOfObjectIDs",
			tpe_obj_object_list, obj);

	return obj;
}

static int 
tpe_obj_object_list(void *data, int eventid, void *event){
	int len,n;
	int *edata;
	struct tpe_obj *obj;

	obj = data;

	printf("Received a list of object IDs\n");
	edata = event;

	/* FIXME: Need to look for deleted objects */
	/* FIXME: Should be able to fix this in TP04 protocol */
	/* FIXME: Should actually check we don't have it */
	n = ntohl(edata[4]);
	len = n * 4 + 4; /* int == 4 on the bus */

	tpe_msg_send(obj->tpe->msg, "MsgGetObjectIDs",NULL, NULL, edata+4, len);

	return 1;
}


