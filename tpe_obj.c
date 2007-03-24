/* * General object related functions.
 *
 * TODO: Better data structure then a linked list 
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_obj.h"
#include "tpe_orders.h"
#include "tpe_event.h"
#include "tpe_msg.h"
#include "tpe_sequence.h"
#include "tpe_util.h"

struct tpe_obj {
	struct tpe *tpe;
	Ecore_List *objs;
	int check;
};



static int tpe_obj_data_receive(void *data, int eventid, void *event);
static void tpe_obj_list_begin(struct tpe *tpe);
static void tpe_obj_list_end(struct tpe *tpe);

static void tpe_obj_cleanup(struct tpe *tpe, struct object *o);

struct tpe_obj *
tpe_obj_init(struct tpe *tpe){
	struct tpe_event *event;
	struct tpe_obj *obj;

	event = tpe->event;

	obj = calloc(1,sizeof(struct tpe_obj));
	obj->tpe = tpe;

	tpe_event_handler_add(event, "MsgObject",
			tpe_obj_data_receive, obj);

	tpe_event_type_add(event, "ObjectNew");
	tpe_event_type_add(event, "ObjectChanged");
	tpe_event_type_add(event, "ObjectDelete");
	tpe_event_type_add(event, "PlanetNoOrders");
	tpe_event_type_add(event, "PlanetColonised");
	tpe_event_type_add(event, "FleetNoOrders");

	tpe_sequence_register(tpe, 
			"MsgGetObjectIDs", 
			"MsgListOfObjectIDs",
			"MsgGetObjectsByID",
			tpe_obj_object_updated,
			tpe_obj_list_begin,
			tpe_obj_list_end);

	obj->objs = ecore_list_new();

	return obj;
}

/**
 * Callback for the event of a object being discovered 
 *
 */
static int
tpe_obj_data_receive(void *data, int eventid, void *edata){
	struct tpe_obj *obj;
	struct object *o,*child;
	int id,n,i;
	int isnew;
	int unused;
	int oldowner;
	void *end;
	
	obj = data;

	edata = ((char *)edata + 16);
	tpe_util_parse_packet(edata, "i", &id);

	isnew = 0;

	o = tpe_obj_obj_get_by_id(obj,id);
	if (o == NULL){
		isnew = 1;
		o = tpe_obj_obj_add(obj,id);
	}

	/* Unlink children */
	for (i = 0 ; i < o->nchildren ; i ++){
		child = tpe_obj_obj_get_by_id(obj,o->children[i]);
		if (child)
			child->parent = 0;
	}

	if (o->name){
		free(o->name);
		o->name = NULL;
	}
	
	if (o->orders){
		for (i = 0 ; i < o->norders ; i ++)
			if (o->orders[i])
				tpe_orders_order_free(o->orders[i]);
		free(o->orders);
	}

	n = tpe_util_parse_packet(edata, "iislllllllaailiip",
			&o->oid, &o->type, &o->name,
			&o->size, 
			&o->pos.x,&o->pos.y,&o->pos.z,
			&o->vel.x,&o->vel.y,&o->vel.z,
			&o->nchildren, &o->children, 
			&o->nordertypes, &o->ordertypes,
			&o->norders,
			&o->updated,&unused,&unused, &end);

	/* Link children */
	for (i = 0 ; i < o->nchildren ; i ++){
		child = tpe_obj_obj_get_by_id(obj,o->children[i]);
		if (!child)
			child = tpe_obj_obj_add(obj,o->children[i]);
		child->parent = id;
	}

	/* Add slots for the orders */
	if (o->norders)
		o->orders = calloc(o->norders, sizeof(struct order *));
	else
		o->orders = NULL;

	/* Handle extra data for different types */
	switch (o->type){
	case OBJTYPE_UNIVERSE:{
		uint32_t turn;
	
		tpe_util_parse_packet(end, "i", &turn);

		obj->tpe->turn = turn;
		
		break;
	}
	case OBJTYPE_GALAXY:
		break;
	case OBJTYPE_SYSTEM:
		break;
	case OBJTYPE_PLANET:
		/* FIXME: Also do resources */
		if (o->planet == NULL){
			o->planet = calloc(1,sizeof(struct object_planet));
			o->owner = -1;
		} 
		oldowner = o->owner;

		tpe_util_parse_packet(end, "iR",
				&o->owner, 
				&o->planet->nresources, &o->planet->resources);

		if (o->owner != oldowner && o->owner != obj->tpe->player)
			tpe_event_send(obj->tpe->event, "PlanetColonised", o,
					tpe_event_nofree, NULL);

		break;
	case OBJTYPE_FLEET:
		if (o->fleet == NULL)
			o->fleet = calloc(1,sizeof(struct object_fleet));

		tpe_util_parse_packet(end, "iSi",
				&o->owner, 
				&o->fleet->nships, &o->fleet->ships,
				&o->fleet->damage);
		break;
	default:
		printf("Unknown object type: %d\n",o->type);
		exit(1);
	}

	tpe_event_send(obj->tpe->event, isnew ? "ObjectNew" : "ObjectChanged",
				o, tpe_event_nofree, NULL);

	/* Check to see if we need to emit a planet or fleet 'no orders'
	 * message */
	if (o->nordertypes != 0 && o->norders == 0){
		if (o->type == OBJTYPE_PLANET)
			tpe_event_send(obj->tpe->event, "PlanetNoOrders", o,
					tpe_event_nofree, NULL);
		else if (o->type == OBJTYPE_FLEET)
			tpe_event_send(obj->tpe->event, "FleetNoOrders", o,
					tpe_event_nofree, NULL);
		else 
			printf("A %d can take orders???\n", o->type);
	}


	return 1;
}

struct object *
tpe_obj_obj_get_by_id(struct tpe_obj *obj, uint32_t oid){
	struct object *o;
	ecore_list_goto_first(obj->objs);
	while ((o = ecore_list_next(obj->objs)))
		if (o->oid == oid)
			return o;

	return NULL;
}

struct object *
tpe_obj_obj_add(struct tpe_obj *obj, int oid){
	struct object *o;

	o = calloc(1,sizeof(struct object));
	o->oid = oid;
	o->tpe = obj->tpe;
	ecore_list_append(obj->objs, o);
	return o;	
}


int
tpe_obj_obj_dump(struct object *o){
	int i;
	printf("Object ID %d Type %d Name: '%s'\n",o->oid,o->type,o->name);
	printf("\tSize %lld\n",o->size);
	printf("\t{ x = %lld, y = %lld, z = %lld }\n",
			o->pos.x, o->pos.y, o->pos.z );
	printf("\t{ dx = %lld, dy = %lld, dz = %lld }\n",
			o->vel.x, o->vel.y, o->vel.z );
	printf("\t%d children: ",o->nchildren);
	for (i = 0 ; i < o->nchildren ;  i ++){
		printf("%d ",o->children[i]);
	}
	printf("\n\t%d orders: ",o->nordertypes);
	for (i = 0 ; i < o->nordertypes ;  i ++){
		printf("%d ",o->ordertypes[i]);
	}
	printf("\n\t%d current orders\n",o->norders);
	for (i = 0 ; i < o->norders ; i ++){
		tpe_orders_order_print(o->tpe, o->orders[i]);
	}

	return 0;
}

Ecore_List *
tpe_obj_obj_list(struct tpe_obj *obj){
	return obj->objs;
}

uint64_t 
tpe_obj_object_updated(struct tpe *tpe, uint32_t oid){
	struct object *obj;

	obj = tpe_obj_obj_get_by_id(tpe->obj,oid);
	if (obj == NULL)
		return 0;

	if (tpe->obj->check)
		obj->ref = tpe->obj->check;	

	return obj->updated;
}


static void
tpe_obj_list_begin(struct tpe *tpe){

	do {
		tpe->obj->check = rand();
	} while (tpe->obj->check == 0);
}

static void
tpe_obj_list_end(struct tpe *tpe){
	struct object *o;
	int check;

	check = tpe->obj->check;

	ecore_list_goto_first(tpe->obj->objs);
	while ((o = ecore_list_next(tpe->obj->objs))){
		if (check != o->ref){
			tpe_event_send(tpe->event, "ObjectDelete", o,
					(void(*)(void*,void*))tpe_obj_cleanup, tpe);
		}
	}
}

static void
tpe_obj_cleanup(struct tpe *tpe, struct object *o){
	int i;

	ecore_list_goto(tpe->obj->objs,o);
	ecore_list_remove(tpe->obj->objs);

	o->tpe = NULL;
	o->oid = -1;
	if (o->name) free(o->name);
	if (o->children) free(o->children);
	if (o->ordertypes) free(o->ordertypes);
	if (o->orders){
		for (i = 0 ; i < o->norders ; i ++)
			tpe_orders_order_free(o->orders[i]);
		free(o->orders);
	}
	
	if (o->fleet){
		if (o->fleet->ships) free(o->fleet->ships);
		free(o->fleet);
	}
	if (o->planet){
		if (o->planet->resources) free(o->planet->resources);
		free(o->planet);
	}
	free(o);
}
