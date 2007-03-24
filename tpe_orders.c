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
#include "tpe_sequence.h"

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
	tpe_event_handler_add(tpe->event, "MsgOrder", 
			tpe_orders_msg_order, tpe);

	tpe_sequence_register(tpe, 
				"MsgGetOrderDescriptionIDs",
				"MsgOrderDescriptionIDs",
				"MsgGetOrderDescription",
				 tpe_orders_order_desc_updated,
				 NULL, NULL);

	/* Register some events?? */


	tpe_event_handler_add(tpe->event, "ObjectChanged", 
			tpe_orders_object_update, tpe);
	tpe_event_handler_add(tpe->event, "ObjectNew", 
			tpe_orders_object_update, tpe);
	

	return orders;
}

uint64_t
tpe_orders_order_desc_updated(struct tpe *tpe, uint32_t id){
	struct order_desc *desc;

	desc = tpe_order_orders_get_desc_by_id(tpe, id);
	if (desc)
		return desc->updated;
	else
		return UINT32_MAX;
}

struct order_desc *
tpe_order_orders_get_desc_by_id(struct tpe *tpe, uint32_t type){
	struct order_desc *od;
	ecore_list_goto_first(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (type == od->otype)
			return od;
	}
	return NULL;

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
tpe_order_get_name_by_type(struct tpe *tpe, uint32_t type){
	struct order_desc *od;
	ecore_list_goto_first(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (type == od->otype)
			return od->name;
	}
	return NULL;
}

static int
tpe_orders_msg_order_description(void *data, int type, void *edata){
	struct tpe *tpe = data;
	int *event = edata;
	struct order_desc *od;

	od = calloc(1,sizeof(struct order_desc));

	event += 4;

	tpe_util_parse_packet(event, "issQl",
			&od->otype,&od->name, &od->description, 
			&od->nargs, &od->args, &od->updated);

	ecore_list_append(tpe->orders->ordertypes, od);

	return 1;
}


/* Update the order list for this object 
 *
 * FIXME: Need to prepare to fragment this packet 
 * 	There may me too many messages for one request 
 */
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

	free(toget);

	return 1;
}

/*
 * FIXME: Optimise - should peak at the end, can not parse an order unless I
 * have to
 */
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
			printf("! Have order for object I don't have: "
					"Obj: %d Slot %d\n",
					order->oid, order->slot);
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
	if (order){
		free(order->resources);
		free(order);
	}
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
 * Returns a string describing the orders on an object.
 *
 * Format is
 * 	<order>Order Name</order>
 * 		<arg>Arg 1</arg>...
 * 
 * @param tpe Tpe structure.
 * @param object Object to print orders of.
 * @return NULL on error, or a string with order description.
 */
const char *
tpe_orders_str_get(struct tpe *tpe, struct object *obj){
	static char buf[BUFSIZ];
	struct order *order;
	int pos;
	int i;

	for (pos = 0 , i = 0 ; i < obj->norders ; i ++){
		order = obj->orders[i];
		if (order == NULL){
			pos += snprintf(buf + pos, BUFSIZ - pos, 
					"<order>Unknown</order>");
			continue;
		}
		pos += snprintf(buf + pos, BUFSIZ - pos, 
				"<order>%s</order>", 
				tpe_order_get_name_by_type(tpe, order->type));
		/* FIXME: Do args... */	
	}

	return buf;
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

	colorder = tpe_order_get_type_by_name(tpe,"Colonise");
	if (colorder == -1)
		return -1; /* FIXME: Temp unavail - should job it to retry */

	return tpe_msg_send_format(tpe->msg, "MsgInsertOrder",
			NULL, NULL,
			"iii00i",
			obj->oid, slot, colorder, what->oid);
}

int 
tpe_orders_object_clear(struct tpe *tpe, struct object *obj){
	int i,rv;
	int *toremove;

	if (obj->norders == 0) return 0;

	toremove = malloc(sizeof(int32_t) * (obj->norders + 2));
	toremove[0] = htonl(obj->oid);
	toremove[1] = htonl(obj->norders);

	for (i = 0 ; i < obj->norders ; i ++){
		toremove[i + 2] = htonl(obj->norders - i - 1);
	}
	
	/* Catch the fail message */
	rv = tpe_msg_send(tpe->msg, "MsgRemoveOrder",
			NULL, NULL,
			toremove, (sizeof(int32_t) * (obj->norders + 2)));

	free(obj->orders);
	obj->orders = NULL;
	obj->norders = 0;

	free(toremove);

	return rv;
}

