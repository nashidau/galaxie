#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>

#include "tpe.h"
#include "ai_smith.h"
#include "tpe_event.h"
#include "tpe_orders.h"
#include "tpe_obj.h"
#include "tpe_msg.h"
#include "tpe_util.h"

struct ai {
	struct tpe *tpe;
};

static int smith_order_planet(void *data, int type, void *event);
static int smith_order_fleet(void *data, int type, void *event);

static int smith_order_insert_cb(void *userdata, const char *msgtype, 
		int len, void *edata);

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

	buf[0] = htonl(o->oid);	 /* What */
	buf[1] = htonl(-1);	 /* Slot */
	buf[2] = htonl(build_id); /* Order type */
	buf[3] = htonl(0);       /* Number of turns == 0 */
	buf[4] = htonl(0);       /* Resource list */
	buf[5] = htonl(0);      /* List : Possible selections */
	buf[6] = htonl(0);	/* List: # items */
	buf[7] = htonl(0);	/* strlen */
	buf[8] = htonl(0);

	tpe_msg_send(smith->tpe->msg, "MsgProbeOrder",
			smith_order_insert_cb, smith,
			buf, 9 * 4);

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
	struct object *o = event;
	struct object *parent;
	struct object *dest;
	struct ai *smith = data;

	printf("$ Smith: Fleet for %s\n",o->name);

	parent = tpe_obj_obj_get_by_id(smith->tpe->obj, o->parent);
	if (!parent) {
		printf("$ Smith: No parent for %s\n",o->name);
		return 1;
	}

	if (parent->type == OBJTYPE_PLANET){
		printf("$ Smith: A planet:  Can I colonise it?\n");
		if (parent->owner == tpe->player){
			printf("Owned by me...\n");
		} else {
			printf("$ smith: Fixme: Colonise this\n");
			return 1;
		}
	}

	/* If here - we need to find somewhere to colonise... */




	return 1;
}
