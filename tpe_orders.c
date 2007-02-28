/* 
 * Very low level system for dealing with order types
 */
#include <arpa/inet.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_event.h"
#include "tpe_msg.h"
#include "tpe_util.h"
#include "tpe_orders.h"
#include "tpe_obj.h"

struct tpe_orders {
	struct tpe *tpe;

	Ecore_List *ordertypes;
};



struct order_desc {
	uint32_t otype;	 /* Type */
	const char *name;
	const char *description;
	
	int nargs;
	struct order_arg *args;

	uint64_t updated;
};

struct order {
	int oid;
	int slot;
	int type;
	int turns;
	int nresources;
	struct build_resources *resources;

	union {
		int colonise;
	} extra;
	
};

static int tpe_orders_msg_order_description_ids(void *, int type, void *data);
static int tpe_orders_msg_order_description(void *, int type, void *data);
static int tpe_orders_msg_order(void *, int type, void *data);

static int tpe_orders_object_update(void *, int type, void *data);

struct tpe_orders *
tpe_orders_init(struct tpe *tpe){
	struct tpe_orders *orders;

	orders = calloc(1,sizeof(struct tpe_orders));
	orders->ordertypes = ecore_list_new();
	orders->tpe = tpe;

	tpe_event_handler_add(tpe->event, "MsgOrderDescription", /* 9 */
			tpe_orders_msg_order_description, tpe);
	tpe_event_handler_add(tpe->event, "MsgOrderDescriptionIDs", /* 33 */
			tpe_orders_msg_order_description_ids, tpe);
	tpe_event_handler_add(tpe->event, "MsgOrder", 
			tpe_orders_msg_order, tpe);

	/* Register some events?? */


	tpe_event_handler_add(tpe->event, "ObjectChanged", 
			tpe_orders_object_update, tpe);
	tpe_event_handler_add(tpe->event, "ObjectNew", 
			tpe_orders_object_update, tpe);
	

	return orders;
}

int
tpe_order_get_type_by_name(struct tpe *tpe, const char *name){
	struct order_desc *od;
	ecore_list_goto_first(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (strcmp(name, od->name) == 0)
			return od->otype;
	}
	return -1;

}

const char * 
tpe_order_get_name_by_type(struct tpe *tpe, int type){
	struct order_desc *od;
	ecore_list_goto_first(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (type == od->otype)
			return od->name;
	}
	return NULL;
}

static int
tpe_orders_msg_order_description_ids(void *data, int type, void *edata){
	struct tpe *tpe = data;
	int *event = edata;
	int noids;
	struct ObjectSeqID *oids = 0;
	int *toget;
	int i,n;
	int seqkey;
	int more;


	event += 4;

	tpe_util_parse_packet(event, "iiO", &seqkey, &more, &noids,&oids);

	toget = malloc(noids * sizeof(int) + 4);
	for (i  = 0 , n = 0; i < noids; i ++){
		toget[n + 1] = htonl(oids[i].oid);
		n ++;
	}
	toget[0] = htonl(n);
	
	tpe_msg_send(tpe->msg, "MsgGetOrderDescription" /* 8 */, NULL, NULL, 
			toget, n * 4 + 4);

	free(toget);
	free(oids);

	return 1;
}

static int
tpe_orders_msg_order_description(void *data, int type, void *edata){
	struct tpe *tpe = data;
	int *event = edata;
	struct order_desc *od;

	od = calloc(1,sizeof(struct order_desc));

	printf("An order description\n");

	event += 4;

	tpe_util_parse_packet(event, "issQl",
			&od->otype,&od->name, &od->description, 
			&od->nargs, &od->args, &od->updated);


	printf("order %d is %s\n\t%s\n",od->otype, od->name,od->description);
	{ int i;
	for (i = 0 ; i < od->nargs ; i ++){
		printf("\tArg: %s  A type %d\n\t%s\n",od->args[i].name,
				od->args[i].arg_type, od->args[i].description);
	}
	}

	ecore_list_append(tpe->orders->ordertypes, od);

	return 1;
}


/* Update the order list for this object */
static int 
tpe_orders_object_update(void *tpev, int type, void *objv){
	struct object *o = objv;
	struct tpe *tpe = tpev;
	int *toget,i;

	assert(o);
	assert(tpe);

	if (o->norders == 0) return 1;

	toget = malloc((o->norders + 2) * sizeof(uint32_t));
	toget[0] = htonl(o->oid);
	toget[1] = htonl(o->norders);

	for (i = 0 ; i < o->norders ; i ++)
		toget[i + 2] = htonl(i);
	
	tpe_msg_send(tpe->msg, "MsgGetOrder", NULL, NULL,
			toget, (o->norders + 2) * sizeof(uint32_t));

	return 1;
}

static int 
tpe_orders_msg_order(void *data, int type, void *event){
	struct tpe *tpe = data;
	struct object *object;
	struct order *order;
	const char *tname;
	void *end;

	order = calloc(1,sizeof(struct order));

	event = (char *)event + 16;
	tpe_util_parse_packet(event, "iiiiBp",
			&order->oid, &order->slot,
			&order->type, &order->turns,
			&order->nresources, &order->resources,
			&end);

	tname = tpe_order_get_name_by_type(tpe, order->type);
	if (tname == NULL){
		printf("Unknown order type %d\n",order->type);
	} else if (strcmp(tname,"Colonise") == 0){
		tpe_util_parse_packet(end, "i", &order->extra.colonise);
	} 
	/* FIXME: Handle all order types - and use order desc to do parsing */

	if (order->slot == -1){
		/* Free it */
		tpe_orders_order_free(order);
	} else {
		object = tpe_obj_obj_get_by_id(tpe->obj, order->oid);
		if (object == NULL){
			tpe_orders_order_free(order);
			return 1;
		}
		if (object->norders < order->slot){
			printf("Weird -order too low\n");
			exit(1);
		}
		if (object->orders[order->slot])
			tpe_orders_order_free(object->orders[order->slot]);
		object->orders[order->slot] = order;
	}

	return 1;
}

int
tpe_orders_order_free(struct order *order){
	free(order->resources);
	free(order);
	return 0;
}

int
tpe_orders_order_print(struct tpe *tpe, struct order *order){
	struct object *target;

	if (order == NULL){
		printf("\tNo order data\n");
		return 1;
	}

	printf("\tOrderSlot %d Type[%2d]: %s\n"
		"\tTurns: %d  Resources: %d\n",
			order->slot, order->type, 
			tpe_order_get_name_by_type(tpe, order->type),
			order->turns,
			order->nresources);
	/* FIXME: Print resources */

	if (order->type == 3){
		printf("Colonising object %d:\n", order->extra.colonise);
		target = tpe_obj_obj_get_by_id(tpe->obj, order->extra.colonise);
		tpe_obj_obj_dump(target);
		printf("End col\n");
	}

	return 0;
}


/**
 * Appends a order to move to a particular location.
 *
 * FIXME: Document
 */
int 
tpe_orders_object_move(struct tpe *tpe, struct object *obj, int slot,
		int64_t x,int64_t y, int64_t z){
	int moveorder;
	
	/* FIXME: Arg check */

	moveorder = tpe_order_get_type_by_name(tpe,"Move");
	if (moveorder == -1)
		return -1; /* FIXME: Temp unavail - should job it to retry */

	return tpe_msg_send_format(tpe->msg, "MsgInsertOrder",
			NULL, NULL,
			"iii00lll",
			obj->oid, slot, moveorder, 
			x,y,z);
}

/**
 * As tpe_orders_object_move, but takes another object as it's destimation.
 *
 * Note that if the destination moves, the order will NOT be updated.
 */
int 
tpe_orders_object_move_object(struct tpe *tpe, struct object *obj, int slot,
		struct object *dest){
	/* FIXME: Arg check */
	return tpe_orders_object_move(tpe, obj, slot, 
			dest->pos.x,dest->pos.y,dest->pos.z);
}
int 
tpe_orders_object_colonise(struct tpe *tpe, struct object *obj, int slot,
		struct object *what){
	int colorder;

	colorder = tpe_order_get_type_by_name(tpe,"Move");
	if (colorder == -1)
		return -1; /* FIXME: Temp unavail - should job it to retry */

	return tpe_msg_send_format(tpe->msg, "MsgInsertOrder",
			NULL, NULL,
			"iii00i",
			obj->oid, slot, colorder, what->oid);
}

