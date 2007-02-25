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
#include "ai_smith.h"
#include "tpe_event.h"
#include "tpe_msg.h"
#include "tpe_orders.h"
#include "tpe_obj.h"
#include "tpe_ship.h"
#include "tpe_util.h"

struct ai {
	struct tpe *tpe;
};


/* Structure embedded into Objects for AI usage */
struct ai_obj {
	int fleet;
};

static int smith_order_planet(void *data, int type, void *event);
static int smith_order_fleet(void *data, int type, void *event);

static int smith_order_insert_cb(void *userdata, const char *msgtype, 
		int len, void *edata);
int order_move(struct tpe *tpe, struct object *o, struct object *dest);

static const char *colonise_order = "Colonise";
static const char *build_order = "BuildFleet";
static const char *move_order = "Move";

static int colonise_id = -1;
static int build_id = -1;
static int move_id = -1;

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

	tpe_event_handler_add(tpe->event, "PlanetNoOrders", 
			smith_order_planet, ai);
	tpe_event_handler_add(tpe->event, "FleetNoOrders",
			smith_order_fleet, ai);

	return ai;
}



static int 
smith_order_planet(void *data, int type, void *event){
	struct object *o = event;
	struct ai *smith = data;
	int buf[100];

	printf("$ Smith: Order for %s\n",o->name);

	if (build_id == -1)
		build_id = tpe_order_get_type_by_name(smith->tpe, build_order);
	assert(build_id != -1);

	buf[0] = htonl(o->oid);	 /* What */
	buf[1] = htonl(-1);	 /* Slot */
	buf[2] = htonl(build_id); /* Order type */
	buf[3] = htonl(0);       /* Number of turns == 0 */
	buf[4] = htonl(0);       /* Resource list */
	buf[5] = htonl(0);      /* List : Possible selections */
	buf[6] = htonl(0);	/* List: # items */
	buf[7] = htonl(0);	/* max len */
	buf[8] = htonl(0);	/* Str: name */
	/* FIXME: NFI what this is */
	buf[9] = htonl(0);	/* Str: name */

//	{int i;
//		for (i = 0 ; i < 9 ; i ++)
//			printf("%08x ",ntohl(buf[i]));
//		printf("\n");
//	}

	tpe_msg_send(smith->tpe->msg, "MsgProbeOrder",
			smith_order_insert_cb, smith,
			buf,  9* 4);

	return 1;
}

static int 
smith_order_insert_cb(void *userdata, const char *msgtype, 
		int len, void *edata){
	struct ai *smith = userdata;
	int *data = edata;
	int oid, slot, type, turns, nbr, noptions = 0;
	struct build_resources *br = 0;
	struct arg_type6 *options = 0;
	const char *str = 0;
	int frigate;
	int i;
	int buf[20];

	/* FIXME: Check result */
	printf("Inserting\n");

	//data += 4;
	/* Evil hard coded-ness again: Here I assume the order type */
	/* FIXME: Re-enable */
	tpe_util_parse_packet(data, "iiii", /* B6s", */
			&oid, &slot,&type,&turns,
			&nbr,&br,
			&noptions,&options,
			&str);
	frigate = 2;
	for (i = 0 ; i < noptions ; i ++){
		if (strcmp(options[i].name,"Frigate") == 0)
			frigate = options[i].id;
	}

	buf[0] = htonl(oid);
	buf[1] = htonl(-1);
	buf[2] = htonl(build_id);
	buf[3] = htonl(0); /* turns */
	buf[4] = htonl(0); /* Resource list */
	buf[5] = htonl(0);
	buf[6] = htonl(1); /* No of items */
	buf[7] = htonl(frigate); /* What */
	buf[8] = htonl(1); /* how many */
	buf[9] = htonl(0); /* Max String  (read only) */
	buf[10] = htonl(0); /* String (name) */

	tpe_msg_send(smith->tpe->msg, "MsgInsertOrder",
			NULL, NULL,
			buf, 11 * 4);


	return 1;
}


/* 
 ?* So there is a fleet with nothing to do hey?
 *
 * Well if we can colonise with it - try it...
 */

static int 
smith_order_fleet(void *data, int type, void *event){
	Ecore_List *all;
	struct object *o = event;
	struct object *parent;
	struct object *dest,*cur;
	uint64_t destdist, curdist;
	struct object_fleet *fleet;
	struct ai *smith = data;
	const char *designtype;
	int i;

	printf("$ Smith: Fleet for %s\n",o->name);

	if (colonise_id == -1)
		colonise_id = tpe_order_get_type_by_name(smith->tpe, 
				colonise_order);
	if (move_id == -1)
		move_id = tpe_order_get_type_by_name(smith->tpe, move_order);
	if (colonise_id == -1 || move_id == -1) 
		return 1;
	/* Smith only cares about frigates... All others are beneath
	 * 	his notice */
	fleet = o->fleet;
	if (fleet == NULL) return 1;

	for (i = 0 ; i < fleet->nships ; i ++){
		designtype = tpe_ship_design_name_get(smith->tpe, 
				fleet->ships[i].design);
		if (strcmp(designtype,"Frigate") == 0)
			break;
	}

	/* Ignoring non-frigate fleet */
	if (i == fleet->nships) return 1;
	

	parent = tpe_obj_obj_get_by_id(smith->tpe->obj, o->parent);
	if (!parent) {
		printf("$ Smith: No parent for %s\n",o->name);
		return 1;
	}

	if (parent->type == OBJTYPE_PLANET){
		printf("$ Smith: A planet:  Can I colonise it?\n");
		if (parent->owner == smith->tpe->player){
			printf("Owned by me...\n");
		} else {
			printf("$ smith: Fixme: Colonise this\n");
			return 1;
		}
	}

	/* If here - we need to find somewhere to colonise... */
	
	printf("$ Seaching for somewhere for %s to colonise\n", o->name);
	all = tpe_obj_obj_list(smith->tpe->obj);
	ecore_list_goto_first(all);
	dest = NULL;
	destdist = UINT64_MAX;
	while ((cur = ecore_list_next(all))){
		if (cur->type != OBJTYPE_PLANET) continue;
		if (dest == NULL && cur->ai == NULL) {
			dest = cur;
			destdist = tpe_util_dist_calc2(dest,o);
			continue;
		}
		if (cur->ai != NULL) continue;
		if (cur->owner == smith->tpe->player) continue;
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

	order_move(smith->tpe,o,dest);
	order_colonise(smith->tpe,o,dest);
	dest->ai = calloc(1,sizeof(struct ai_obj));
	dest->ai->fleet = o->oid;
	return 1;
}


int
order_move(struct tpe *tpe, struct object *o, struct object *dest){
	int buf[20];
	uint64_t x,y,z;

	buf[0] = htonl(o->oid);		/* Who are are ordering */
	buf[1] = htonl(-1);		/* Slot */
	buf[2] = htonl(move_id);	/* Order type */
	buf[3] = htonl(0);		/* Turns */
	buf[4] = htonl(0);		/* Resource list */
	*(uint64_t *)(buf +5) = htonll(dest->pos.x);
	*(uint64_t *)(buf +7) = htonll(dest->pos.y);
	*(uint64_t *)(buf +9) = htonll(dest->pos.z);

	tpe_util_parse_packet(buf +5,"lll",&x,&y,&z);
	if (dest->pos.x != x || dest->pos.y != y || dest->pos.z != z){
		printf("Error: %lld != %lld || %lld != %lld || %lld != %lld\n",
				dest->pos.x,x,dest->pos.y,y,dest->pos.z,z);
		exit(0);
	}


	

	tpe_msg_send(tpe->msg, "MsgInsertOrder",
			NULL, NULL,
			buf, 11 * 4);

	return 0;	
}

int
order_colonise(struct tpe *tpe, struct object *o, struct object *dest){
	int buf[20];

	buf[0] = htonl(o->oid);		/* Who are are ordering */
	buf[1] = htonl(-1);		/* Slot */
	buf[2] = htonl(colonise_id);	/* Order type */
	buf[3] = htonl(0);		/* Turns */
	buf[4] = htonl(0);		/* Resource list */
	buf[5] = htonl(dest->oid);	/* What to colonise */


	tpe_msg_send(tpe->msg, "MsgInsertOrder",
			NULL, NULL,
			buf, 6 * 4);

	return 0;
}
