/* 
 * General object related functions.
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
#include "tpe_event.h"
#include "tpe_msg.h"
#include "tpe_util.h"

struct tpe_obj {
	struct tpe *tpe;
	Ecore_List *objs;
};



static int tpe_obj_object_list(void *data, int eventid, void *event);
static int tpe_obj_data_receive(void *data, int eventid, void *event);

struct tpe_obj *
tpe_obj_init(struct tpe *tpe){
	struct tpe_event *event;
	struct tpe_obj *obj;

	event = tpe->event;

	obj = calloc(1,sizeof(struct tpe_obj));
	obj->tpe = tpe;

	tpe_event_handler_add(event, "MsgListOfObjectIDs",
			tpe_obj_object_list, obj);
	tpe_event_handler_add(event, "MsgObject",
			tpe_obj_data_receive, obj);

	tpe_event_type_add(event, "ObjectNew");
	tpe_event_type_add(event, "ObjectChanged");
	tpe_event_type_add(event, "PlanetNoOrders");
	tpe_event_type_add(event, "FleetNoOrders");

	obj->objs = ecore_list_new();

	return obj;
}

/* OID Seqence
 *
 * Handles message 31
 *
 */
static int 
tpe_obj_object_list(void *data, int eventid, void *event){
	struct tpe_obj *obj;
	int seqkey, more;
	int noids;
	struct ObjectSeqID *oids = NULL;
	struct object *o;
	int *toget,i,n;

	obj = data;
	oids = 0;

	/* FIXME */
	event = ((char *)event + 16);

	tpe_util_parse_packet(event, "iiO", &seqkey, &more, &noids,&oids);
	printf("Seqkey is %d\n",seqkey);
	/* FIXME: handle 'more' */
	printf("# objects to go %d\n",more);
	printf("# objects to go %d\n",noids);

	toget = malloc(noids * sizeof(int) + 4);
	for (i  = 0 , n = 0; i < noids; i ++){
		o = tpe_obj_obj_get_by_id(obj,oids[i].oid);
		if (o == NULL || o->updated < oids[i].updated){
			toget[n + 1] = htonl(oids[i].oid);
			n ++;
		}
	}
	toget[0] = htonl(n);

	tpe_msg_send(obj->tpe->msg, "MsgGetObjectsByID",NULL, NULL, 
				toget, n * 4 + 4);

	free(toget);

	return 1;
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
	for (i = 0 ; i < o->nchildren ; o ++){
		child = tpe_obj_obj_get_by_id(obj,o->children[i]);
		if (child)
			child->parent = 0;
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
	for (i = 0 ; i < o->nchildren ; o ++){
		child = tpe_obj_obj_get_by_id(obj,o->children[i]);
		if (!child)
			child = tpe_obj_obj_add(obj,o->children[i]);
		child->parent = id;
	}

	/* Handle extra data for different types */
	switch (o->type){
	case OBJTYPE_UNIVERSE:{
		uint32_t turn;
	
		tpe_util_parse_packet(end, "i", &turn);

		/* FIXME: Do something with it */
		
		break;
	}
	case OBJTYPE_GALAXY:
		break;
	case OBJTYPE_SYSTEM:
		break;
	case OBJTYPE_PLANET:
		/* FIXME: Also do resources */
		if (o->planet == NULL)
			o->planet = calloc(1,sizeof(struct object_planet));

		tpe_util_parse_packet(end, "iR",
				&o->owner, 
				&o->planet->nresources, &o->planet->resources);

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

	//tpe_obj_obj_dump(o);

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
tpe_obj_obj_get_by_id(struct tpe_obj *obj, int oid){
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

	return 0;
}

