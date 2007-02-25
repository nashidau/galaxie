/*
 * For now just manages the ship names,
 * eventually should get designs and all that jazz.
 */
#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <Ecore.h>
#include <Ecore_Data.h>
#include <Evas.h>

#include "tpe.h"
#include "tpe_event.h"
#include "tpe_msg.h"
#include "tpe_util.h"
#include "tpe_ship.h"

struct tpe_ship {
	Ecore_List *designs;
};	
struct design {
	int did;
	const char *name;
	const char *description;
};

static int tpe_ship_msg_design_list(void *data, int eventid, void *event);
static int tpe_ship_msg_design(void *data, int eventid, void *event);

struct tpe_ship *
tpe_ship_init(struct tpe *tpe){
	struct tpe_ship *ships;
	struct tpe_event *event;

	event = tpe->event;

	ships = calloc(1,sizeof(struct tpe_ship));
	ships->designs = ecore_list_new();
	tpe_event_handler_add(event, "MsgListOfDesignIDs",
			tpe_ship_msg_design_list, tpe);
	tpe_event_handler_add(event, "MsgDesign",
			tpe_ship_msg_design, tpe);

	return ships;
}

const char *
tpe_ship_design_name_get(struct tpe *tpe, uint32_t design){
	struct design *d;
	ecore_list_goto_first(tpe->ship->designs);
	while ((d = ecore_list_next(tpe->ship->designs)))
		if (d->did == design)
			return d->name;

	return NULL;
}


/* FIXME: This is cut and paste */
static int 
tpe_ship_msg_design_list(void *data, int eventid, void *event){
	struct tpe *tpe;
	int seqkey, more;
	int noids;
	struct ObjectSeqID *oids = 0;
	int *toget;
	int i,n;
	const char *name;

	tpe = data;

	/* FIXME */
	event = ((char *)event + 16);

	tpe_util_parse_packet(event, "iiO", &seqkey, &more, &noids,&oids);

	toget = malloc(noids * sizeof(int) + 4);
	for (i  = 0 , n = 0; i < noids; i ++){
		name = tpe_ship_design_name_get(tpe, oids[i].oid);
		if (name == NULL){
			toget[n + 1] = htonl(oids[i].oid);
			n ++;
		}
	}

	if (n != 0){ 
		/* Get what we need */
		toget[0] = htonl(n);

		tpe_msg_send(tpe->msg, "MsgGetDesign",NULL, NULL, 
				toget, n * 4 + 4);
	}

	free(toget);
	free(oids);

	return 1;

}
static int 
tpe_ship_msg_design(void *data, int eventid, void *event){
	struct design *design = 0;
	int64_t unusedl;
	int32_t *cats = 0,ncats;
	struct tpe *tpe = data;

	design = calloc(1,sizeof(struct design));

	/* FIXME */
	event = ((char *)event + 16);

	tpe_util_parse_packet(event, "ilass", &design->did, 
			&unusedl, &ncats,&cats, 
			&design->name,
			&design->description);
	ecore_list_append(tpe->ship->designs, design);

	free(cats);

	return 1;
}
