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
#include "tpe_sequence.h"
#include "tpe_ship.h"

struct tpe_ship {
	Ecore_List *designs;
};	
struct design {
	uint32_t did;
	const char *name;
	const char *description;
	uint64_t updated;
};

static int tpe_ship_msg_design(void *data, int eventid, void *event);

struct tpe_ship *
tpe_ship_init(struct tpe *tpe){
	struct tpe_ship *ships;
	struct tpe_event *event;

	event = tpe->event;

	ships = calloc(1,sizeof(struct tpe_ship));
	ships->designs = ecore_list_new();

	tpe_sequence_register(tpe, "MsgGetDesignIDs",
			"MsgListOfDesignIDs",
			"MsgGetDesign",
			tpe_ship_design_updated_get,
			NULL, NULL);

	tpe_event_handler_add(event, "MsgDesign",
			tpe_ship_msg_design, tpe);

	return ships;
}

struct design *
tpe_ship_design_get(struct tpe *tpe, uint32_t design){
	struct design *d;
	ecore_list_goto_first(tpe->ship->designs);
	while ((d = ecore_list_next(tpe->ship->designs)))
		if (d->did == design)
			return d;

	return NULL;

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


uint64_t 
tpe_ship_design_updated_get(struct tpe *tpe, uint32_t design){
	struct design *d;

	d = tpe_ship_design_get(tpe,design);
	if (d)
		return d->updated;
	return 0;
}

static int 
tpe_ship_msg_design(void *data, int eventid, void *event){
	struct design *design = 0;
	int32_t *cats = 0,ncats;
	struct tpe *tpe = data;

	design = calloc(1,sizeof(struct design));
	/* FIXME */
	event = ((char *)event + 16);

	tpe_util_parse_packet(event, "ilass", &design->did, 
			&design->updated, &ncats,&cats, 
			&design->name,
			&design->description);

	ecore_list_append(tpe->ship->designs, design);

	free(cats);

	return 1;
}
