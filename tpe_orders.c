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
#include "server.h"
#include "tpe_util.h"
#include "tpe_orders.h"
#include "tpe_obj.h"
#include "tpe_sequence.h"

enum {
	/* As defined by tp03 protocol */
	ARG_COORD 	= 0,
	ARG_TIME 	= 1,
	ARG_OBJECT 	= 2,
	ARG_PLAYER 	= 3,
	ARG_RELCOORD 	= 4,
	ARG_RANGE	= 5,
	ARG_LIST 	= 6,
	ARG_STRING 	= 7,
	ARG_REFERENCE   = 8,
	ARG_REFERENCE_LIST = 9,
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
	uint32_t turns;
	uint32_t max;
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
/* Type 4: ARG_RELCOORD */
struct order_arg_relcoord {
	uint32_t obj;
	int64_t x,y,z;
};

/* Type 5: ARG_RANGE */
struct order_arg_range {
	int32_t value;
	int32_t min,max;
	int32_t inc;
};

/* Type 6: ARG_LIST */
struct order_arg_list_option {
	int id;
	int max;
	char *option;
};
struct order_arg_list_selection {
	uint32_t selection;
	uint32_t count;
};
struct order_arg_list {
	uint32_t noptions; /* # of Options */
	struct order_arg_list_option *options;
	uint32_t nselections;
	struct order_arg_list_selection *selections;
};


/* Type 7: ARG_STRING */ 
struct order_arg_string {
	uint32_t maxlen;
	char *str;
};

/* Type 8: ARG_REFERENCE */
/* FIXME: Not implemented */

union order_arg_data {
	struct order_arg_coord coord;
	struct order_arg_time time;
	struct order_arg_object object;
	struct order_arg_player player;
	struct order_arg_relcoord relcoord;
	struct order_arg_range range;
	struct order_arg_list list;
	struct order_arg_string string;
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
		struct order_desc *desc, int *p, int *end);

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
		return UINT64_MAX;
}

struct order_desc *
tpe_order_orders_get_desc_by_id(struct tpe *tpe, uint32_t type){
	struct order_desc *od;
	ecore_list_first_goto(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (type == od->otype)
			return od;
	}
	return NULL;

}

int
tpe_order_get_type_by_name(struct tpe *tpe, const char *name){
	struct order_desc *od;
	ecore_list_first_goto(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (strcmp(name, od->name) == 0)
			return od->otype;
	}
	return -1;

}

const char * 
tpe_order_get_name_by_type(struct tpe *tpe, uint32_t type){
	struct order_desc *od;
	ecore_list_first_goto(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (type == od->otype)
			return od->name;
	}
	return NULL;
}

const char *
tpe_order_get_name(struct tpe *tpe, struct order *order){
	struct order_desc *od;
	assert(order);
	ecore_list_first_goto(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (order->type == od->otype)
			return od->name;
	}
	return "Unknown";
}


/**
 * Parse an order 
 *
 */
static int
tpe_orders_msg_order_description(void *data, int type, void *edata){
	struct msg *msg;
	struct tpe *tpe = data;
	struct order_desc *od;

	od = calloc(1,sizeof(struct order_desc));

	msg = edata;

	tpe_util_parse_packet(msg->data, msg->end, "issQl",
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

		server_send(o->server, "MsgGetOrder", NULL, NULL,
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
	struct order *order = NULL;
	struct msg *msg;
	void *end;
	
	msg = event;

	order = calloc(1,sizeof(struct order));

	tpe_util_parse_packet(msg->data, msg->end, "iiiiBp",
			&order->oid, &order->slot,
			&order->type, &order->turns,
			&order->nresources, &order->resources,
			&end);

	desc = tpe_order_orders_get_desc_by_id(tpe, order->type);
	if (desc)
		tpe_order_parse_args(tpe, order, desc, end, msg->end);
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


		printf("Received order on Object: %d\n%s\n",order->oid, 
				tpe_orders_str_get(tpe,object));
	}


	return 1;
}

/**
 * Parse the arguments for an order.
 *
 */
static void
tpe_order_parse_args(struct tpe *tpe, struct order *order, 
		struct order_desc *desc, int *p, int *end){
	int i,j;
	union order_arg_data *data;

	/* FIXME: Leak?? */
	order->args = calloc(desc->nargs, sizeof(union order_arg_data));

	/* FIXME: Should do a little more checking here */
	for (i = 0 ; i < desc->nargs ; i ++){
		data = calloc(1,sizeof(union order_arg_data));
		order->args[i] = data;
		switch(desc->args[i].arg_type){
		case ARG_COORD:
			tpe_util_parse_packet(p,end,"lllp",
					&data->coord.x,
					&data->coord.y,
					&data->coord.z,
					&p);
			break;
		case ARG_TIME:
			tpe_util_parse_packet(p,end,"iip",&data->time.turns,
					&data->time.max,&p);
			break;
		case ARG_OBJECT:
			tpe_util_parse_packet(p,end,"ip",
					&data->object.oid,
					&p);
			break;
		case ARG_PLAYER:
			tpe_util_parse_packet(p,end,"iip",
					&data->player.pid,
					&data->player.flags);
			break;
		case ARG_RELCOORD:
			tpe_util_parse_packet(p,end,"illlp",
					&data->relcoord.obj,
					&data->relcoord.x,
					&data->relcoord.y,
					&data->relcoord.z,&p);
			break;
		case ARG_RANGE:
			tpe_util_parse_packet(p,end,"iiii",
					&data->range.value,
					&data->range.min,
					&data->range.max,
					&data->range.inc,
					&p);
			break;
		case ARG_LIST:
			tpe_util_parse_packet(p,end,"ip", 
					&data->list.noptions,&p);
			/* FIXME: Temp sanity check */
			assert(data->list.noptions < 1000);
			/* Allocate a buffer of the right size */
			data->list.options = calloc(data->list.noptions,	
					sizeof(struct order_arg_list_option));
			for (j = 0 ; j < data->list.noptions ; j ++){
				struct order_arg_list_option *op;
				op = data->list.options + j;
				tpe_util_parse_packet(p,end,"isip",
						&op->id,&op->option,&op->max,
						&p);
			}
			/* FIXME: Check selected option is a valid one */
			tpe_util_parse_packet(p,end,"ip",&data->list.nselections,&p);
			data->list.selections = calloc(data->list.nselections,
				       sizeof(struct order_arg_list_selection));
			for (j = 0 ; j < data->list.nselections ; j ++){
				struct order_arg_list_selection *sel;
				sel = data->list.selections + j;
				tpe_util_parse_packet(p,end,"iip",
						&sel->selection,&sel->count,&p);
			}
			break;
		case ARG_STRING:
			tpe_util_parse_packet(p, end,"isp", 
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

	if (obj == NULL) return NULL;

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
	int i,j;

	switch (desc->args[argnum].arg_type){
	case ARG_COORD:
		pos += snprintf(buf + pos, maxlen - pos,
				"<arg>%lld %lld %lld</arg>", 
				order->args[argnum]->coord.x,
				order->args[argnum]->coord.y,
				order->args[argnum]->coord.z);
		break;

	case ARG_TIME:{
		struct order_arg_time *time;
		time = &order->args[argnum]->time;
		pos += snprintf(buf + pos, maxlen - pos,
				"<arg>%d turns (of %d</arg>",
				time->turns, time->max);

		} break;
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
		pos += snprintf(buf+pos,maxlen-pos,
				"<arg>Player - unimplemented</arg>");
		break;
	case ARG_RELCOORD:{
		struct order_arg_relcoord *rc;
		struct object *obj;
		const char *name;
		rc = &order->args[argnum]->relcoord;
		obj = tpe_obj_obj_get_by_id(tpe,rc->obj);
		if (obj == NULL) name = "Unknown";
		else name = obj->name;

		pos += snprintf(buf+pos,maxlen-pos,
				"<arg>%s</arg>"
				"<arg>+(%llx,%llx,%llx)</arg>",
				name,
				rc->x,rc->y,rc->z);
		}
		break;
	case ARG_RANGE:{
		struct order_arg_range *range;

		range = &order->args[argnum]->range;
		pos += snprintf(buf+pos,maxlen-pos,
				"<arg>%d (Max: %d Inc %d)</arg>",
				range->value,range->max,range->inc);
		break;
	}
	case ARG_LIST:{
		struct order_arg_list *list;
		list = &order->args[argnum]->list;
		for (i = 0 ; i < list->nselections ; i ++){
			for (j = 0 ; j < list->noptions ; j ++){
				if (list->selections[i].selection!=list->options[j].id)
					continue;
				pos += snprintf(buf + pos, maxlen - pos,
					"<arg>%s x %d</arg>",
					list->options[j].option,
					list->selections[i].count);
				break;
			}
		}
		break;
	}
	case ARG_STRING:
		pos += snprintf(buf + pos, maxlen - pos, 
				"<arg>%s</arg>", 
				order->args[argnum]->string.str);
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
	return server_send_format(obj->server, "MsgInsertOrder",
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

	return server_send_format(obj->server, "MsgInsertOrder",
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
		return server_send_format(obj->server, "MsgInsertOrder",
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
	rv = server_send(obj->server, "MsgRemoveOrder",
			NULL, NULL,
			toremove, (sizeof(int32_t) * (obj->norders + 2)));

	free(obj->orders);
	obj->orders = NULL;
	obj->norders = 0;

	free(toremove);

	return rv;
}

