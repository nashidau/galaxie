#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <Evas.h>
#include <Ecore.h>

#include "tpe.h"
#include "ai_smith.h"
#include "tpe_event.h"
#include "tpe_orders.h"
#include "tpe_obj.h"
#include "tpe_msg.h"

struct ai {
	struct tpe *tpe;
};

static int smith_order_planet(void *data, int type, void *event);
static int smith_order_fleet(void *data, int type, void *event);

static int smith_order_insert_cb(void *userdata, const char *msgtype, 
		int len, void *edata);

static const char *colonise_order = "Colonise";
static const char *build_order = "Build Fleet";
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
	buf[6] = htonl(1);	/* List: # items */
	buf[7] = htonl(2);	  /* Type: Guessing 2 for frigate */
	buf[8] = htonl(1);	  /* Count : 1 */
	buf[9] = htonl(0);	/* Strlen */
	
	tpe_msg_send(smith->tpe->msg, "MsgInsertOrder",
			smith_order_insert_cb, smith,
			buf, 10 * 4);

	return 1;
}

static int 
smith_order_insert_cb(void *userdata, const char *msgtype, 
		int len, void *edata){
	int *data = edata;

	printf("$ smith: Response was %d\n", ntohl(data[2]));

	return 1;
}



static int 
smith_order_fleet(void *data, int type, void *event){
	struct object *o = event;
	struct ai *smith = data;

	printf("$ Smith: Fleet for %s\n",o->name);
	return 1;
}
