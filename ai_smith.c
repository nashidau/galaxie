#include <arpa/inet.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Eina.h>

#include "tpe.h"
#include "galaxietypes.h"
#include "ai_util.h"
#include "server.h"
#include "tpe_event.h"
#include "tpe_orders.h"
#include "objects.h"
#include "tpe_ship.h"
#include "tpe_util.h"

/** General AI struture for Smith
 */
struct ai {
	/** Pointer back to TPE structure */
	struct tpe *tpe;

	/* Number of last ship built */
	int shipid;
};

TPE_AI("Smith", "Smith: A basic AI for minisec", ai_smith_init)


/* Structure embedded into Objects for AI usage */
struct ai_obj {
	int fleet;
};

static int smith_order_planet(void *data, int type, void *event);
static int smith_order_fleet(void *data, int type, void *event);
static int smith_planet_colonised(void *data, int type, void *event);

static int smith_order_insert_cb(void *userdata, struct msg *msg);
int order_move(struct tpe *tpe, struct object *o, struct object *dest);
int order_colonise(struct tpe *tpe, struct object *o, struct object *dest);

static const char *build_order = "BuildFleet";

static int build_id = -1;

static const char *shipnames[] = {
	"Endeavour",
	"Spore",
	"Mayflower",
	"Colonial",
	"Vanquish",
	"Victory",
	"Sirius",
};
#define N_SHIPNAMES (sizeof(shipnames)/sizeof(shipnames[0]))

struct ai *ai_smith_init(struct tpe *);

/* Move:
 * 	pos : Absoluate space coords <int64,int64,int64>
 * Colonose: 
 * 	planet : Object ID
 * BuildFleet
 * 	ships type 6
 * 	name : String (len, data)
 */
struct ai *
ai_smith_init(struct tpe *tpe){
	struct ai *ai;

	ai = calloc(1,sizeof(struct ai));
	ai->tpe = tpe;
	ai->shipid = 0;

	tpe_event_handler_add("PlanetNoOrders", smith_order_planet, ai);
	tpe_event_handler_add("FleetNoOrders", smith_order_fleet, ai);
	tpe_event_handler_add("PlanetColonised", smith_planet_colonised, ai);

	return ai;
}



static int 
smith_order_planet(void *data, int type, void *event){
	struct object *o = event;
	struct ai *smith = data;

	printf("$ Smith: Order for %s\n",o->name);
	printf("$ OID: %d  notypes %d norders %d\n", o->oid, o->nordertypes,
			o->norders);
	{ int i;
		for (i = 0 ; i < o->nordertypes ; i ++){
			printf("$\t%d: %s\n",o->ordertypes[i],
					tpe_order_get_name_by_type(smith->tpe, 
							o->ordertypes[i]));
		}

	}

	if (build_id == -1){
		build_id = tpe_order_get_type_by_name(smith->tpe, build_order);
		/* FIXME: Should requeue or something */
		build_id = 2;
//		if (build_id == -1)
//			return 1;
	}

	if (o->server == NULL){
		/* Big FIXME */
		printf("$ Smith: Can't do anything\n");
		printf("$ Smith: Object data is lacking server\n");
		printf("$ Smith: Please Fix this!\n");
		return 0;
	}

	server_send_format(o->server,  "MsgProbeOrder",
			smith_order_insert_cb, smith,
			"iii000000", o->oid, -1, build_id);

	return 1;
}

static int 
smith_order_insert_cb(void *userdata, struct msg *msg){
	struct ai *smith = userdata;
	int oid, slot, type, turns, nbr, noptions = 0;
	struct build_resources *br = NULL;
	struct arg_type6 *options = NULL;
	int maxstr;
	char *str = NULL;
	int frigate;
	int i;

	if (strcmp(msg->type,"MsgFail") == 0){
		/* FIXME: Start cleaning up? */
		printf("Message failure for insert order\n");
		return 1;
	}

	/* FIXME: Check result */
	tpe_util_parse_packet(msg->data, msg->end, "iiiiB6i", 
			&oid, &slot,&type,&turns,
			&nbr,&br,
			&noptions,&options,
			&maxstr);

	for (i = 0, frigate = 0 ; i < noptions ; i ++){
		if (strcmp(options[i].name,"Frigate") == 0){
			frigate = options[i].id;
			break;
		}
	}
	if (i == noptions){
		printf("Failed to find Frigate type - can't colonise\n");
		return 0;
	}

	if (maxstr > 100 || maxstr < 1) maxstr = 30;
	str = malloc(maxstr);
	snprintf(str, maxstr, "%s #%d", shipnames[rand() % N_SHIPNAMES],
			smith->shipid ++);
	
	server_send_format(msg->server, "MsgInsertOrder",
		NULL, NULL,
		"iii000iii0s",
		oid, -1, build_id, 1, frigate, 1, str);

	free(br);
	/* FIXME: Need ways to auto free these */
	for (i = 0, frigate = 0 ; i < noptions ; i ++)
		free(options[i].name);
	free(options);
	free(str);

	return 1;
}


/* 
 ?* So there is a fleet with nothing to do hey?
 *
 * Well if we can colonise with it - try it...
 */

static int 
smith_order_fleet(void *data, int type, void *event){
	Eina_List *all;
	struct object *o = event;
	struct object *parent;
	struct object *dest,*cur;
	uint64_t destdist, curdist;
	struct object_fleet *fleet;
	struct ai *smith = data;
	const char *designtype;
	int i,total;
	uint32_t oid;

	printf("$ Smith: Orders for Fleet %s\n",o->name);

	/* Smith only cares about frigates... All others are beneath
	 * 	his notice */
	/* FIXME: This needs to be done better */
	fleet = o->fleet;
	if (fleet == NULL) return 1;

	for (i = 0 ; i < fleet->nships ; i ++){
		designtype = tpe_ship_design_name_get(smith->tpe, 
				fleet->ships[i].design);
		/* FIXME: Need to requeue this item */
		if (designtype == NULL)
			continue;
		if (strcmp(designtype,"Frigate") == 0)
			break;
	}

	/* Ignoring non-frigate fleet */
	if (i == fleet->nships) return 1;
	

	parent = tpe_obj_obj_get_by_id(smith->tpe, o->parent);
	if (!parent) {
		printf("$ Smith: No parent for %s\n",o->name);
		return 1;
	}

	if (parent->type == OBJTYPE_PLANET){
		if (parent->owner == -1){
			tpe_orders_object_colonise(smith->tpe, o, SLOT_LAST, 
					parent);
	
			return 1;
		}
	}

	/* If here - we need to find somewhere to colonise... */
	
	printf("$ Seaching for somewhere for %s to colonise\n", o->name);
	all = tpe_obj_obj_list(smith->tpe->obj);
	ecore_list_first_goto(all);
	total = ecore_list_count(all);
	dest = NULL;
	destdist = UINT64_MAX;
	while ((total > ecore_list_index(all))){
		oid = (uint32_t)ecore_list_next(all);
		cur = tpe_obj_obj_get_by_id(smith->tpe, oid);
		if (cur->type != OBJTYPE_PLANET) continue;
		if (dest == NULL && cur->ai == NULL) {
			dest = cur;
			destdist = tpe_util_dist_calc2(dest,o);
			continue;
		}
		if (cur->ai != NULL) continue;
		if (cur->owner >= 0) continue;
		curdist = tpe_util_dist_calc2(cur,o);
		if (curdist < destdist){
			/* New target */
			destdist = curdist;
			dest = cur;
		}
	}

	if (dest == NULL){
		printf("$ Smith: No where to send them too!\n");
		return 1;
	}

	printf("$: Sending %s(%d) to %s\n",o->name, o->oid,dest->name);	

	tpe_orders_object_move_object(smith->tpe, o, SLOT_LAST, dest);
	tpe_orders_object_colonise(smith->tpe, o, SLOT_LAST, dest);
	
	dest->ai = calloc(1,sizeof(struct ai_obj));
	dest->ai->fleet = o->oid;
	return 1;
}

/**
 * Event handler for planet colonised.
 *
 * Someone colonised a planet - maybe I'm sending a ship there?  If I am,
 * redirect the ship somewhere else.  No point trying ot invade, a single
 * colonising vessel will almost certainly be destroyed.
 */
static int 
smith_planet_colonised(void *data, int type, void *event){
	struct object *o;
	struct object *colfleet;
	struct object *dest;
	struct ai *smith;
	struct ai_obj *ai;

	o = event;
	smith = data;

	/* Wasn't planning on colonising... */
	if (o->ai == NULL) return 1;

	ai = o->ai;

	colfleet = tpe_obj_obj_get_by_id(smith->tpe, ai->fleet);
	if (colfleet == NULL){
		/* Fleet was destroyed - probably while trying to colonised */
		free(o->ai);
		o->ai = NULL;
		return 1;
	}

	dest = ai_util_planet_closest_uncolonised(smith->tpe, colfleet);

	/* FIXME: Should do something more intelligent */
	/* Currently I'll just let it try anyway */
	if (dest == NULL)
		return 1;

	tpe_orders_object_clear(smith->tpe, o);

	tpe_orders_object_move_object(smith->tpe, colfleet, SLOT_LAST, dest);
	tpe_orders_object_colonise(smith->tpe, colfleet, SLOT_LAST, dest);
	
	dest->ai = calloc(1,sizeof(struct ai_obj));
	dest->ai->fleet = o->oid;

	return 1;	
}
