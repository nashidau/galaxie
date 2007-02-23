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
printf("$ build id is %d  What is %d\n",build_id, o->oid);
	buf[0] = htonl(o->oid);	 /* What */
	buf[1] = htonl(-1);	 /* Slot */
	buf[2] = htonl(build_id); /* Order type */
	buf[3] = htonl(0);       /* Number of turns == 0 */
	buf[4] = htonl(0);       /* Resource list */
	buf[5] = htonl(0);      /* List : Possible selections */
	buf[6] = htonl(0);	/* List: # items */
	buf[7] = htonl(0);	/* strlen */
	buf[8] = htonl(0);
#if 0
	buf[7] = htonl(2);	  /* Type: Guessing 2 for frigate */
	buf[8] = htonl(1);	  /* Count : 1 */
	buf[9] = htonl(0);	/* Strlen */
#endif
/*
	tpe_msg_send(smith->tpe->msg, "MsgInsertOrder",
			smith_order_insert_cb, smith,
			buf, 10 * 4);
*/
	tpe_msg_send(smith->tpe->msg, "MsgProbeOrder",
			smith_order_insert_cb, smith,
			buf, 9 * 4);

	return 1;
}

static int 
smith_order_insert_cb(void *userdata, const char *msgtype, 
		int len, void *edata){
	int *data = edata;
	int oid, slot, type, turns, nbr, noptions;
	struct build_resources *br = 0;
	struct arg_type6 *options = 0;
	const char *str = 0;
	int i;

	printf("$ smith: Response was %d\n", ntohl(data[2]));
	
	/* FIXME: Check result */

	data += 4;
	/* Evil hard coded-ness again: Here I assume the order type */
	tpe_util_parse_packet(data, "iiii", /* B6s */
			&oid, &slot,&type,&turns,
			&nbr,&br,
			&noptions,&options,
			&str);

	printf("%d %d %d %d\n", oid, slot, type, turns);
#if 0
	printf("Resources are:\n");
	for (i = 0 ; i < nbr ; i ++){
		printf("\t%d: '%d'\n",
				br[i].rid, br[i].cost);
	}
	printf("Options are: \n");
	for (i = 0 ; i < noptions ; i ++){
		printf("\t%d: '%s' (max %d)\n",
				options[i].id, options[i].name, options[i].max);
	}
	
#endif

	return 1;
}



static int 
smith_order_fleet(void *data, int type, void *event){
	struct object *o = event;
	struct ai *smith = data;

	printf("$ Smith: Fleet for %s\n",o->name);
	return 1;
}
