/* 
 * Very low level system for dealing with order types
 */
#include <arpa/inet.h>
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
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

enum {
	ARG_COORD = 0,
	ARG_TIME = 1,
	ARG_OBJECT = 2,
	ARG_PLAYER = 3,
	ARG_STRING = 7,
};

struct tpe_orders {
	struct tpe *tpe;

	Ecore_List *ordertypes;
};



/* Type 0: ARG_COORD */
struct order_arg_coord {
	int64_t x,y,z;
};
/* Type 1: ARG_TIME */
struct order_arg_time {
	int64_t turns;
	int64_t max;
};
/* Type 2: ARG_OBJECT */
struct order_arg_object {
	uint32_t oid;
};
/* Type 3: ARG_PLAYER */
struct order_arg_player {
	uint32_t pid;
	uint32_t flags;
};


/* Type 7: ARG_STRING */ 
struct order_arg_string {
	uint32_t maxlen;
	char *str;
};

union order_arg_data {
	struct order_arg_coord coord;
	struct order_arg_time time;
	struct order_arg_object object;
	struct order_arg_player player;
	struct order_arg_string string;
};

struct order_desc {
	uint32_t otype;	 /* Type */
	const char *name;
	const char *description;
	
	int nargs;
	struct order_arg *args; /* The type of the args */

	uint64_t updated;
};



struct order {
	int oid;
	int slot;
	int type;
	int turns;
	int nresources;
	struct build_resources *resources;
	
	/* These come from the order desc */
	union order_arg_data **args;
};

static int tpe_orders_msg_order_description(void *, int type, void *data);
static int tpe_orders_msg_order(void *, int type, void *data);

static int tpe_orders_object_update(void *, int type, void *data);

static int tpe_order_arg_format(struct tpe *tpe, char *buf, int pos, int maxlen,
		struct order *order, struct order_desc *desc, int argnum);
static void tpe_order_parse_args(struct tpe *tpe, struct order *order, 
		struct order_desc *desc, int *p);

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

/**
 * Parse an order 
 *
 */
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
 */
static int 
tpe_orders_object_update(void *tpev, int type, void *objv){
	struct object *o = objv;
	struct tpe *tpe = tpev;
	int *toget,i;
	int frag,fragments, items;

	assert(o);
	assert(tpe);

	if (o->norders == 0) return 1;

	fragments = (o->norders + 99)/ 100; /* XXX magic */ 	

	for (frag = 0 ; frag < fragments ; frag ++){
		if (frag == fragments - 1)
			items = o->norders % 100;
		else 
			items = 100;
		toget = malloc((items + 2) * sizeof(uint32_t));

		toget[0] = htonl(o->oid);
		toget[1] = htonl(items);

		for (i = 0 ; i < items ; i ++)
			toget[i + 2] = htonl(i + frag * 100);

		tpe_msg_send(tpe->msg, "MsgGetOrder", NULL, NULL,
				toget, (items + 2) * sizeof(uint32_t));

		free(toget);
	}

	return 1;
}

/**
 * Callback for receiving an order from the server.
 *
 * FIXME: Optimise - should peak at the end, can not parse an order unless I
 * have to
 *
 */
static int 
tpe_orders_msg_order(void *data, int type, void *event){
	struct tpe *tpe = data;
	struct object *object;
	struct order_desc *desc;
	struct order *order;
	void *end;

	order = calloc(1,sizeof(struct order));

	event = (char *)event + 16;
	tpe_util_parse_packet(event, "iiiiBp",
			&order->oid, &order->slot,
			&order->type, &order->turns,
			&order->nresources, &order->resources,
			&end);

	desc = tpe_order_orders_get_desc_by_id(tpe, order->type);
	if (desc)
		tpe_order_parse_args(tpe, order, desc, end);
	else {
		printf("No description for order type %d\n",order->type);
	}
	
	if (order->slot == -1){
		/* Free it */
		tpe_orders_order_free(order);
	} else {
		object = tpe_obj_obj_get_by_id(tpe, order->oid);
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

/**
 * Parse the arguments for an order.
 *
 */
static void
tpe_order_parse_args(struct tpe *tpe, struct order *order, 
		struct order_desc *desc, int *p){
	int i;
	union order_arg_data *data;

	/* FIXME: Leak?? */
	order->args = calloc(desc->nargs, sizeof(union order_arg_data));

	/* FIXME: Should do a little more checking here */
	for (i = 0 ; i < desc->nargs ; i ++){
		data = calloc(1,sizeof(union order_arg_data));
		order->args[i] = data;

		switch(desc->args[i].arg_type){
		case ARG_COORD:
			tpe_util_parse_packet(p,"lllp",
					&data->coord.x,
					&data->coord.y,
					&data->coord.z,
					&p);
			break;
		case ARG_TIME:
			break;
		case ARG_OBJECT:
			tpe_util_parse_packet(p, "ip",
					&data->object.oid,
					&p);
			break;
		case ARG_PLAYER:
			break;
		case ARG_STRING:
			tpe_util_parse_packet(p, "isp", 
					&data->string.maxlen,
					&data->string.str,
					&p);
			break;
		}
	}


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
	struct order_desc *desc;
	int pos;
	int i,j;

	buf[0] = 0;

	for (pos = 0 , i = 0 ; i < obj->norders ; i ++){
		order = obj->orders[i];
		if (order == NULL){
			pos += snprintf(buf + pos, BUFSIZ - pos, 
					"<order>Unknown</order>");
			continue;
		}
		desc = tpe_order_orders_get_desc_by_id(tpe, order->type);

		pos += snprintf(buf + pos, BUFSIZ - pos, 
				"<order>%s</order>", 
				tpe_order_get_name_by_type(tpe, order->type));

		for (j = 0 ; j < desc->nargs ; j ++){
			pos = tpe_order_arg_format(tpe, buf, pos, BUFSIZ, 
					order, desc, j);	
		}

	}

	return buf;
}

static int
tpe_order_arg_format(struct tpe *tpe, char *buf, int pos, int maxlen,
		struct order *order, struct order_desc *desc, int argnum){
	struct object *obj;

	switch (desc->args[argnum].arg_type){
	case ARG_COORD:
		pos += snprintf(buf + pos, maxlen - pos,
				"<arg>%lld %lld %lld</arg>", 
				order->args[argnum]->coord.x,
				order->args[argnum]->coord.y,
				order->args[argnum]->coord.z);
		break;

	case ARG_TIME:

		break;
	case ARG_OBJECT:
		obj = tpe_obj_obj_get_by_id(tpe,
				order->args[argnum]->object.oid);
		if (obj)
			pos += snprintf(buf + pos, maxlen - pos,
					"<arg>%d: %s</arg>",obj->oid,obj->name);
		else
			pos += snprintf(buf + pos, maxlen - pos,
					"<arg>%d [Unknown]</arg>",
					order->args[argnum]->object.oid);
		break;
	case ARG_PLAYER:

	case ARG_STRING:
		pos += snprintf(buf + pos, maxlen - pos, 
				"<arg>%s</arg>", "arg");
		break;
	default: 
		printf("Don't handle arg type %d yet\n", desc->args[argnum].arg_type);


	}

	return pos;

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


/**
 * Send a build order in the standard format.
 *
 * The var_arg format is:
 *  Type
 *  Number
 * A type of -1 ends the list.
 *
 * @param  tpe TPE structure
 * @param obj to do building
 * @param Slot to insert into
 * @param Name of hte object
 * @return -1 on error, 0 on sucess.
 */
int 
tpe_orders_object_build(struct tpe *tpe, struct object *obj, int slot,
		const char *name, ...){
	va_list ap;
	uint32_t type, count;
	int *buf;
	int items;
	int buildorder;
	int i;


	buildorder = tpe_order_get_type_by_name(tpe,"BuildFleet");

	/* First pass - count */
	va_start(ap,name);
	items = 0;
	do {
		type = va_arg(ap, uint32_t);
		if (type == (uint32_t)-1 || type == 0)
			break;
		count = va_arg(ap, uint32_t);
		if (count == 0)
			break;
		items ++;
	} while (1);
	va_end(ap);

	if (items){
		va_start(ap,name);
		buf = malloc(sizeof(uint32_t) * items * 2 + sizeof(uint32_t));
		buf[0] = htonl(items);
		for (i = 0 ; i < items; i ++){
			buf[i * 2 + 1] = htonl(va_arg(ap, uint32_t));
			buf[i * 2 + 2] = htonl(va_arg(ap, uint32_t));
		}
		va_end(ap);
		return tpe_msg_send_format(tpe->msg, "MsgInsertOrder",
					NULL, NULL,
					"V" "iii000r0s",
					obj->oid, slot,
					buildorder,
					items * 2 + 1, buf,
					name);
	}

	return -1;
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

