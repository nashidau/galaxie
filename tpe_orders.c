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
#include <talloc.h>

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

/* For sending with probe orders */
/* FIXME: This needs to go - use argsize below */
static const int argsizesempty[] = {
	/* ARG_COORD */   sizeof(int64_t) * 3,
	/* ARG_TIME  */   sizeof(int32_t) * 2,
	/* ARG_OBJECT */  sizeof(int32_t),
	/* ARG_PLAYER */  sizeof(int32_t) * 2,
	/* ARG_RELCOORD*/ sizeof(int32_t) + sizeof(int64_t) * 3,
	/* ARG_RANGE */   sizeof(int32_t) * 4,
	/* ARG_LIST */    sizeof(int32_t) * 4,
	/* ARG_STRING */  sizeof(int32_t) * 2,
	/* ARG_REF */     sizeof(int32_t) * 3,
	/* ARG_REFLIST */ sizeof(int32_t) * 2,
};


static int arg_coord_write(struct order_arg *, union order_arg_data *, char *);
static int arg_time_write(struct order_arg *, union order_arg_data *, char *);
static int arg_player_write(struct order_arg *, union order_arg_data *, char *);
static int arg_relcoord_write(struct order_arg *, union order_arg_data *, char *);
static int arg_range_write(struct order_arg *, union order_arg_data *, char *);
static int arg_list_size_get(struct order_arg *, union order_arg_data *);
static int arg_list_write(struct order_arg *, union order_arg_data *,char*);
static int arg_string_size_get(struct order_arg *, union order_arg_data *);
static int arg_string_write(struct order_arg *, union order_arg_data *, char *);
static int arg_object_write(struct order_arg *, union order_arg_data *, char *);
static const struct argsize {
	int empty;
	int (*wdata)(struct order_arg *, union order_arg_data *);
	int (*write)(struct order_arg *, union order_arg_data *, char *buf);
} argsizes[] = {
	{ /* ARG_COORD */   sizeof(int64_t) * 3,	NULL,arg_coord_write },
	{ /* ARG_TIME  */   sizeof(int32_t) * 2,	NULL,arg_time_write },
	{ /* ARG_OBJECT */  sizeof(int32_t),		NULL,arg_object_write },
	{ /* ARG_PLAYER */  sizeof(int32_t) * 2,	NULL,arg_player_write },
	{ /* ARG_RELCOORD*/ sizeof(int32_t) + sizeof(int64_t) * 3, NULL,
							arg_relcoord_write },
	{ /* ARG_RANGE */   sizeof(int32_t) * 4,	NULL,arg_range_write },
	{ /* ARG_LIST */    sizeof(int32_t) * 4,	arg_list_size_get,
							arg_list_write },
	{ /* ARG_STRING */  
	  sizeof(int32_t) * 2,	arg_string_size_get, arg_string_write },
	{ /* ARG_REF */     sizeof(int32_t) * 3,	NULL,NULL },
	{ /* ARG_REFLIST */ sizeof(int32_t) * 2,	NULL,NULL },

};


struct tpe_orders {
	struct tpe *tpe;

	Ecore_List *ordertypes;
};



/* Used by tpe_orders_object_probe call */
struct probeinfo {
	struct order_desc *desc;
	struct object *obj;

	void (*cb)(void *, struct object *, struct order_desc *,struct order *);
	void *data;
};

static int tpe_orders_msg_order_description(void *, int type, void *data);
static int tpe_orders_msg_order(void *, int type, void *data);

static int tpe_orders_object_update(void *, int type, void *data);

static int tpe_order_arg_format(struct tpe *tpe, char *buf, int pos, int maxlen,
		struct order *order, struct order_desc *desc, int argnum);
static void tpe_order_parse_args(struct tpe *tpe, struct order *order, 
		struct order_desc *desc, int *p, int *end);

static int object_probe_data(void *userdata, struct msg *msg);

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

	if (order->args){
		free(order->args);
		order->args = NULL;
	}

	order->args = calloc(desc->nargs, sizeof(union order_arg_data));
	if (order->args == NULL){
		perror(__FILE__ "calloc");
		return;
	}

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
		default:
			printf("Unimplemented arg type: %d!\n",
					desc->args[i].arg_type);
		}
	}


}

int
tpe_orders_order_free(struct order *order){
	/* FIXME: Leaks lots */
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

/** 
 * Sends a probe order to the server for a particular object/order type
 * combination.
 *
 * Sets the callback for when the data arrives so the client may handle it
 * themselves if they wish.
 */
int 
tpe_orders_object_probe(struct tpe *tpe, struct object *obj,uint32_t otype,
		void (*cb)(void *, struct object *, struct order_desc *,
				struct order *), void *udata){
	int i;
	struct probeinfo *probeinfo;
	struct order_desc *desc;
	int size;
	uint32_t *buf;
	
	assert(tpe); assert(obj);
	assert(obj->server);

        for (i = 0 ; i < obj->nordertypes ; i ++){
                if (obj->ordertypes[i] == otype)
                        break;
        }
        assert(i < obj->nordertypes);
        if (i == obj->nordertypes){
                printf("Couldn't find ordertype on object\n");
		return -1;
        }

	desc = tpe_order_orders_get_desc_by_id(obj->tpe, otype);	

	probeinfo = calloc(1,sizeof(struct probeinfo));
	probeinfo->desc = desc;
	probeinfo->obj = obj;
	probeinfo->cb = cb;
	probeinfo->data = udata;

	/* Pass 1: Work out size */
	for (i = 0, size = 0 ; i < desc->nargs ; i ++){
		size += argsizesempty[desc->args[i].arg_type];
	}
	size /= 4;

	buf = calloc(size,sizeof(int32_t));

	return server_send_format(obj->server, "MsgProbeOrder",
			object_probe_data, probeinfo, 
			"iii00r", obj->oid, -1 /* Slot */, otype,
			size, buf);

}

static int
object_probe_data(void *userdata, struct msg *msg){
	struct probeinfo *probeinfo = userdata;
	struct order *order;
	struct object *obj;
	int *argstart;

	assert(userdata); assert(msg);
	printf("Got probe msg callback\n");

	assert(strcmp(msg->type,"MsgFail"));

	obj = probeinfo->obj;

	order = calloc(1,sizeof(struct order));

	/* FIXME: Check */
	tpe_util_parse_packet(msg->data, msg->end, "iiiiBp",
			&order->oid, &order->slot,
			&order->type, &order->turns,
			&order->nresources, &order->resources,
			&argstart);

	tpe_order_parse_args(obj->tpe, order, probeinfo->desc, 
			argstart,msg->end); 

	tpe_orders_order_print(obj->tpe, order);	

	if (probeinfo->cb)
		probeinfo->cb(probeinfo->data, probeinfo->obj, 
				probeinfo->desc, order);

	return 0;
}

/*
 * Takes an order structure, either from a probe, or from a current list, and
 * submits it to the server.
 */
int
tpe_orders_order_update(struct tpe *tpe, struct order *order){
	struct order_desc *desc;
	assert(tpe);
	assert(order);
	int i;
	int len;
	char *buf;
	
	desc = tpe_order_orders_get_desc_by_id(tpe, order->type);

	for (i = 0, len = 0 ; i < desc->nargs ; i ++){
		if (argsizes[desc->args[i].arg_type].wdata == NULL)
			len += argsizes[desc->args[i].arg_type].empty;
		else
			len += argsizes[desc->args[i].arg_type].wdata(
					desc->args + i, order->args[i]);
		
	}
	buf = calloc(1,len);
	for (i = 0, len = 0 ; i < desc->nargs ; i ++){
		len += argsizes[desc->args[i].arg_type].write(
				desc->args + i, order->args[i], buf + len);
	}



	{ 
		struct object *obj;
		obj = tpe_obj_obj_get_by_id(tpe,order->oid);
		
	server_send_format(obj->server, "MsgInsertOrder", 
			NULL, NULL,
			"Viii00r", order->oid, order->slot, order->type, 
			len / 4, buf);
	}

	return 0;
}

static int 
arg_coord_write(struct order_arg *orderarg, union order_arg_data *orderdata, 
		char *buf){
	struct order_arg_coord *coord;
	int64_t tmp[3];

	coord = &(orderdata->coord);

	tmp[0] = ntohll(coord->x);
	tmp[1] = ntohll(coord->y);
	tmp[2] = ntohll(coord->z);

	memcpy(buf, tmp, 3 * sizeof(uint64_t));

	return  3 * sizeof(uint64_t);
}
static int 
arg_time_write(struct order_arg *orderarg, union order_arg_data *orderdata, 
		char *buf){
	struct order_arg_time *tm;
	uint32_t tmp[2];

	tm = &(orderdata->time);
	tmp[0] = htonl(tm->turns);
	tmp[1] = 0;
	memcpy(buf, tmp, sizeof(uint32_t) * 2);
	return 2 * sizeof(uint32_t);	
}
static int 
arg_player_write(struct order_arg *orderarg, union order_arg_data *orderdata, 
		char *buf){
	struct order_arg_player *play;
	uint32_t tmp[2];

	play = &(orderdata->player);
	tmp[0] = htonl(play->pid);
	tmp[1] = 0;
	memcpy(buf, tmp, sizeof(uint32_t) * 2);
	return sizeof(uint32_t) * 2;
}
static int 
arg_relcoord_write(struct order_arg *orderarg, union order_arg_data *orderdata,
		char *buf){
	struct order_arg_relcoord *relcoord;
	int64_t tmp[3];
	int32_t itmp;

	relcoord = &(orderdata->relcoord);
	itmp = htonl(relcoord->obj);
	tmp[0] = htonl(relcoord->x);
	tmp[1] = htonl(relcoord->y);
	tmp[2] = htonl(relcoord->z);
	memcpy(buf, &itmp, sizeof(int32_t));
	memcpy(buf + sizeof(int32_t), tmp, sizeof(int64_t) * 3);

	return sizeof(int32_t) + 3 * sizeof(int64_t);
}

static int 
arg_range_write(struct order_arg *orderarg, union order_arg_data *orderdata, 
		char *buf){
	struct order_arg_range *range;
	int32_t tmp;

	range = &(orderdata->range);
	tmp = htonl(range->value);
	memcpy(buf, &tmp, sizeof(int32_t));
	memset(buf + sizeof(int32_t), 0, sizeof(int32_t) * 3);

	return sizeof(int32_t) * 4;
}

static int 
arg_list_size_get(struct order_arg *arg, union order_arg_data *orderdata){
	return orderdata->list.nselections * sizeof(uint32_t) * 2 + 
			sizeof(uint32_t) * 4;
}


static int 
arg_list_write(struct order_arg *arg, union order_arg_data *orderdata, char *buf){
	struct order_arg_list *list;
	int32_t *tmp;
	int i;

	list = &(orderdata->list);

	tmp = calloc(sizeof(uint32_t),
			2 * list->nselections + 4);
	tmp[3] = htonl(list->nselections);
	for (i = 0 ; i < list->nselections ; i ++){
		printf("%d %d\n",
		(list->selections[i].selection),
		(list->selections[i].count));
		tmp[4 + i * 2] = htonl(list->selections[i].selection);
		tmp[4 + i * 2 + 1] = htonl(list->selections[i].count);
	}

	memcpy(buf,tmp,(2 * list->nselections + 4) * sizeof(uint32_t));
	free(tmp);

	return (2 * list->nselections + 4) * sizeof(uint32_t);
}
static int 
arg_string_size_get(struct order_arg *arg, union order_arg_data *data){
	struct order_arg_string *str;
	int len;
	assert(arg); assert(data);

	str = &(data->string);

	if (!str->str)
		return sizeof(uint32_t) * 2;	
	
	len = strlen(str->str);
	if (len > str->maxlen)
		len = str->maxlen;
	/* For TP03 this is a hack, for TP04 we shouldn't do this */
	if (len % 4)
		len += 4 - len % 4;
	assert(len % 4 == 0);
	return sizeof(uint32_t) * 2 + len;
}

static int 
arg_string_write(struct order_arg *arg, union order_arg_data *data, char *buf){
	struct order_arg_string *str;
	int len;
	int *ibuf;
	int slen;

	len = arg_string_size_get(arg, data);
	str = &(data->string);

	if (str->str){
		slen = strlen(str->str);
		if (slen % 4) slen += 4 - slen % 4;
	} else 
		slen = 0;


	ibuf = (void*)buf;
	*ibuf ++ = 0; /* Maxlen: Read-only */
	*ibuf ++ = htonl(slen);
	strncpy((char*)ibuf, str->str, slen);

	return len;
}


static int 
arg_object_write(struct order_arg *arg, union order_arg_data *data, char *buf){
	int *ibuf;

	ibuf = (void*)buf;
	*ibuf = htonl(data->object.oid);
	return 4;
}

