/**
 * Jones is an agressive AI - sepecifically designed to attack other players.
 *
 * Assumptions:
 * 	* Homeworlds are worth 5 battleships
 *
 * Algorithm... build 7 battleships
 * 		Send to attack a homeworld
 * 		build 1 frigate send to target world
 *
 */
#include <arpa/inet.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Data.h>

#include "tpe.h"
#include "ai_util.h"
#include "tpe_event.h"
#include "server.h"
#include "tpe_orders.h"
#include "tpe_obj.h"
#include "tpe_ship.h"
#include "tpe_util.h"


/** General AI struture for Jones
 */
struct ai {
	/** Pointer back to TPE structure */
	struct tpe *tpe;

	struct server *server;
};

static int jones_order_fleet(void *data, int type, void *event);
static int jones_order_planet(void *data, int type, void *event);
static int jones_order_insert_cb(void *userdata, struct msg *msg);

TPE_AI("jones", "Jones - Aggresive Minisec AI", ai_jones_init)

struct ai *
ai_jones_init(struct tpe *tpe){
	struct ai *ai;
	
	ai = calloc(1,sizeof(struct ai));
	ai->tpe = tpe;

	tpe_event_handler_add(tpe->event, "PlanetNoOrders",
			jones_order_planet, ai);
	tpe_event_handler_add(tpe->event, "FleetNoOrders",
			jones_order_fleet, ai);

	return ai;
}

/**
 * Jones should really only need one planet
 */
static int
jones_order_planet(void *aiv, int type, void *planetv){
	struct ai *jones = aiv;
	struct object *planet;
	int buildid;

	planet = planetv;

	buildid = tpe_order_get_type_by_name(jones->tpe, "BuildFleet");
	if (buildid == -1) return 1;
	
	server_send_format(jones->server, "MsgProbeOrder",
			jones_order_insert_cb, jones,
			"iii000000", planet->oid, -1, buildid);

	return 1;
}

/**
 * Handler for the probem order response 
 */
static int 
jones_order_insert_cb(void *userdata, struct msg *msg){
	struct ai *jones = userdata;
	int frigate, battleship;
	int oid,slot,type,turns,nbr,noptions,i,maxstr;
	struct build_resources *br = NULL;
	struct arg_type6 *options = NULL;
	struct object *obj;

	if (strcmp(msg->type, "MsgOrder") != 0){
		printf("Got a %s back!\n", msg->type);
		exit(1);
	}

	tpe_util_parse_packet(msg->data, msg->end, "iiiiB6i", 
			&oid, &slot,&type,&turns,
			&nbr,&br,
			&noptions,&options,
			&maxstr);

	for (i = 0, frigate = 0, battleship = 0 ; i < noptions ; i ++){
		if (strcmp(options[i].name,"Frigate") == 0){
			frigate = options[i].id;
		}
		if (strcmp(options[i].name,"Battleship") == 0){
			battleship = options[i].id;
		}
	}

	if (frigate == 0 || battleship == 0){
		printf("Could not find frigate or battleship\n");
		exit(1);
	}

	obj = tpe_obj_obj_get_by_id(jones->tpe, oid);

	tpe_orders_object_build(jones->tpe, obj, -1, 
			"Kill Fleet #9",
			battleship, 7, 
			0, 0);
	tpe_orders_object_build(jones->tpe, obj, -1,
			"Col Fleet #9",
			frigate, 1,
			0, 0);
	return 1;
}
	
static int
jones_order_fleet(void *aiv, int type, void *fleetv){
	struct object *obj = fleetv;
	struct object_fleet *fleet;
	struct ai *jones = aiv;
	int i;
	const char *designtype;

	fleet = obj->fleet;

	if (fleet == NULL) return 1;
	for (i = 0 ; i < fleet->nships ; i ++){
		designtype = tpe_ship_design_name_get(jones->tpe,
				fleet->ships[i].design);
		if (designtype == NULL) continue;
		if (strcmp(designtype, "Frigate") == 0 || 
				strcmp(designtype,"Battleship") == 0)
			break;
	}

	if (strcmp(designtype,"Frigate") == 0){

	} else if (strcmp(designtype,"Battleship") == 0){

	}
	
	return 1;
}
