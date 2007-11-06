/* 
 * General object related functions.
 *
 * Objects are inserted into two data structures -
 * 	a linked list
 * 	a hash table
 */
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_obj.h"
#include "tpe_orders.h"
#include "tpe_event.h"
#include "server.h"
#include "tpe_resources.h"
#include "tpe_sequence.h"
#include "tpe_util.h"

struct tpe_obj {
	struct tpe *tpe;
	Ecore_Hash *objhash;
	int check;

	/* Special objects */
	struct object *home;
};

struct object otmp;
#define OFFSET(field) ((char*)&otmp - (char*)&otmp.field)

struct parseitem objparse[] = {
	{ PARSETYPE_INT, OFFSET(oid), 0, NULL },
	{ PARSETYPE_INT, OFFSET(type), 0, NULL },
	{ PARSETYPE_STRING, OFFSET(name), 0, NULL },
	{ PARSETYPE_LONG, OFFSET(pos.x), 0, NULL },
	{ PARSETYPE_LONG, OFFSET(pos.y), 0, NULL },
	{ PARSETYPE_LONG, OFFSET(pos.z), 0, NULL },
	{ PARSETYPE_LONG, OFFSET(vel.x), 0, NULL },
	{ PARSETYPE_LONG, OFFSET(vel.y), 0, NULL },
	{ PARSETYPE_LONG, OFFSET(vel.z), 0, NULL },
	{ PARSETYPE_ARRAYOF | PARSETYPE_INT, 
			OFFSET(children), OFFSET(nchildren), NULL },
	{ PARSETYPE_ARRAYOF | PARSETYPE_INT, 
			OFFSET(ordertypes), OFFSET(nordertypes), NULL },
	{ PARSETYPE_INT, OFFSET(type), 0, NULL },
	{ PARSETYPE_LONG, OFFSET(updated), 0, NULL },
	{ PARSETYPE_INT, -1, 0, NULL },
	{ PARSETYPE_INT, -1, 0, NULL },
	{ PARSETYPE_END, -1, 0, NULL },
};


const char *const object_magic = "ObjectMagic";

static int tpe_obj_data_receive(void *data, int eventid, void *event);
static void tpe_obj_list_begin(struct tpe *tpe);
static void tpe_obj_list_end(struct tpe *tpe);


static void tpe_obj_object_description_list_begin(struct tpe *tpe);
static void tpe_obj_object_description_list_end(struct tpe *tpe);
static uint64_t tpe_obj_object_description_updated(struct tpe *, uint32_t odid);

static void tpe_obj_cleanup(struct tpe *tpe, struct object *o);

static void tpe_obj_home_check(struct tpe *tpe, struct object *o);

static void tpe_obj_update_children(struct tpe *tpe, struct object *o, int noldchildren, int *oldchildren);


//static int tpe_obj_hash_compare_data(const void *, const void *);
//static int tpe_obj_hash_func(const void *);
//	ecore_hash_dump_graph

struct tpe_obj *
tpe_obj_init(struct tpe *tpe){
	struct tpe_event *event;
	struct tpe_obj *obj;

	event = tpe->event;

	obj = calloc(1,sizeof(struct tpe_obj));
	obj->tpe = tpe;

	tpe_event_handler_add(event, "MsgObject",
			tpe_obj_data_receive, tpe);

	tpe_event_type_add(event, "ObjectNew");
	tpe_event_type_add(event, "ObjectChanged");
	tpe_event_type_add(event, "ObjectDelete");
	tpe_event_type_add(event, "PlanetNoOrders");
	tpe_event_type_add(event, "PlanetColonised");
	tpe_event_type_add(event, "FleetNoOrders");

	tpe_sequence_register(tpe,
			"MsgGetObjectDescriptionIDs",
			"MsgListOfObjectDescriptionIDs",
			"MsgGetObjectDescriptionsByID",
			tpe_obj_object_description_updated,
			tpe_obj_object_description_list_begin,
			tpe_obj_object_description_list_end);

	tpe_sequence_register(tpe, 
			"MsgGetObjectIDs", 
			"MsgListOfObjectIDs",
			"MsgGetObjectsByID",
			tpe_obj_object_updated,
			tpe_obj_list_begin,
			tpe_obj_list_end);

	obj->objhash = ecore_hash_new(NULL,NULL);

	return obj;
}

/**
 * Callback for the event of a object being discovered 
 *
 */
static int
tpe_obj_data_receive(void *data, int eventid, void *edata){
	struct msg *msg;
	struct tpe *tpe;
	struct tpe_obj *obj;
	struct object *o;
	int *oldchildren,noldchildren;
	int id,n,i;
	int isnew;
	void *end;
	
	tpe = data;
	obj = tpe->obj;
	msg = edata;

	tpe_util_parse_packet(msg->data, msg->end, "i", /* i */ &id);
	isnew = 0;

	o = tpe_obj_obj_get_by_id(tpe,id);
	if (o == NULL){
		isnew = 1;
		o = tpe_obj_obj_add(obj,id);
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


	if (msg->protocol == 3){
		/* Save children */
		oldchildren = o->children;
		o->children = NULL;
		noldchildren = o->nchildren;

		n = tpe_util_parse_packet(msg->data, msg->end, 
				"iislllllllaail--p",
				&o->oid, &o->type, &o->name,
				&o->size, 
				&o->pos.x,&o->pos.y,&o->pos.z,
				&o->vel.x,&o->vel.y,&o->vel.z,
				&o->nchildren, &o->children, 
				&o->nordertypes, &o->ordertypes,
				&o->norders,
				&o->updated,NULL, NULL, &end);
		tpe_obj_update_children(tpe, o,noldchildren,oldchildren);
	} else if (msg->protocol == 4){
		/* TP 04 */
		n = tpe_util_parse_packet(msg->data, msg->end, 
				"iissill",
				&o->oid, &o->type, &o->name, &o->description,
				&o->parent,  
				&o->nchildren, &o->children, 
				&o->updated);
	} else {
		printf("Unknown protocol %d\n",msg->protocol);
		return 1;
	}
			

		/* Add slots for the orders */
	if (o->norders)
		o->orders = calloc(o->norders, sizeof(struct order *));
	else
		o->orders = NULL;

	/* Handle extra data for different types */
#warning Fix this soon
#if 0
	switch (o->type){
	case OBJTYPE_UNIVERSE:{
		uint32_t turn;
		tpe_util_parse_packet(end, msg->end, "i", &turn);

		obj->tpe->turn = turn;
		printf("Updated universe: Turn %d\n",turn);	
		
		break;
	}
	case OBJTYPE_GALAXY:
		break;
	case OBJTYPE_SYSTEM:
		break;
	case OBJTYPE_PLANET:
		if (o->planet == NULL){
			o->planet = calloc(1,sizeof(struct object_planet));
			o->owner = -1;
		} 
		oldowner = o->owner;

		tpe_util_parse_packet(end, msg->end, "iR",
				&o->owner, 
				&o->planet->nresources, &o->planet->resources);

		if (o->owner != oldowner && o->owner != obj->tpe->player)
			tpe_event_send(obj->tpe->event, "PlanetColonised", o,
					tpe_event_nofree, NULL);

		if (obj->home == NULL)
			tpe_obj_home_check(tpe, o);

		break;
	case OBJTYPE_FLEET:
		if (o->fleet == NULL)
			o->fleet = calloc(1,sizeof(struct object_fleet));
		/* FIXME: XXX: TODO: this is mithro's bug about bad messages */
		/* This is one of the places triggering overflow in 
		 * tpe_util */
		tpe_util_parse_packet(end, msg->end, "iSi",
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
#endif
	return 1;
}

struct object *
tpe_obj_obj_get_by_id(struct tpe *tpe, uint32_t oid){
	if (tpe == NULL) return NULL;
	if (tpe->obj == NULL) return NULL;
	if (oid == TPE_OBJ_NIL) return NULL;

	return ecore_hash_get(tpe->obj->objhash, (void*)oid);
}

struct object *
tpe_obj_obj_add(struct tpe_obj *obj, int oid){
	struct object *o;

	assert(oid != TPE_OBJ_NIL);
	if (oid == TPE_OBJ_NIL) return NULL;

	o = calloc(1,sizeof(struct object));
	o->magic = object_magic;
	o->oid = oid;
	o->tpe = obj->tpe;
	ecore_hash_set(obj->objhash, (void*)oid, o);
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

/* This needs to be removed */
Ecore_List *
tpe_obj_obj_list(struct tpe_obj *obj){
	if (obj == NULL) return NULL;
	return ecore_hash_keys(obj->objhash);
}


/**
 * Returns a list of all objects of a specified type
 *
 * FIXME: this list is invalidated as soon as an object is deleted.
 */
struct bytype {
	Ecore_List *list;
	enum objtype type;
};

static void
_append_by_type(void *nodev, void *typedata){
	struct bytype *type = typedata;
	Ecore_Hash_Node *node = nodev;
	struct object *o;

	o = node->value;
	/* Should insert in order */
	if (o->type == type->type)
		ecore_list_append(type->list,o);
}


Ecore_List *
tpe_obj_obj_list_by_type(struct tpe *tpe, enum objtype type){
	struct bytype bytype;

	bytype.list = ecore_list_new();
	bytype.type = type;

	ecore_hash_for_each_node(tpe->obj->objhash, _append_by_type, 
			&bytype);
	
	return bytype.list;
}

uint64_t 
tpe_obj_object_updated(struct tpe *tpe, uint32_t oid){
	struct object *obj;

	obj = tpe_obj_obj_get_by_id(tpe,oid);
	if (obj == NULL)
		return 0;

	if (tpe->obj->check)
		obj->ref = tpe->obj->check;	

	return obj->updated;
}

/**
 * Get the sibling (either next or previous) of an object.
 *
 * If it is the last (or first) this function returns NULL.
 *
 * @param obj Object to get sibling off.
 * @param next Get the next sbiling (0 == prev)
 * @return Sibling or NULL on error 
 */
struct object *
tpe_obj_obj_sibling_get(struct tpe *tpe, struct object *obj, int next){
	struct object *parent;
	int i;

	if (obj == NULL) return NULL;

	if (obj->parent == -1){
		printf("Objects %d (%s) parent is NULL!\n", obj->oid,obj->name);
		return NULL;
	}

	parent = tpe_obj_obj_get_by_id(tpe, obj->parent);
	if (parent == NULL) return NULL;

	for (i = 0 ; i < parent->nchildren ; i ++){
		if (parent->children[i] == obj->oid)
			break;
	}
	if (i == 0 && next == 0) return NULL;
	if (i == parent->nchildren) return NULL;
	if (i == parent->nchildren -1 && next) return NULL;

	if (next)
		return tpe_obj_obj_get_by_id(tpe,parent->children[i + 1]);
	else
		return tpe_obj_obj_get_by_id(tpe,parent->children[i - 1]);
}

struct object *
tpe_obj_obj_child_get(struct tpe *tpe, struct object *obj){
	if (tpe == NULL) return NULL;
	if (obj == NULL) return NULL;
	if (obj->nchildren == 0) return NULL;

	return tpe_obj_obj_get_by_id(tpe, obj->children[0]);
}

struct object *
tpe_obj_obj_parent_get(struct tpe *tpe, struct object *obj){
	if (tpe == NULL) return NULL;
	if (obj == NULL) return NULL;
	if (obj->parent == -1) return NULL;

	return tpe_obj_obj_get_by_id(tpe, obj->parent);
}

static void
tpe_obj_list_begin(struct tpe *tpe){

	do {
		tpe->obj->check = rand();
	} while (tpe->obj->check == 0);
}

struct listcbdata {
	struct tpe *tpe;
	int check;
};

static void
tpe_obj_list_cleanup_cb(void *nodev, void *checkdata){
	struct listcbdata *cbdata;
	Ecore_Hash_Node *node = nodev;
	struct object *o;

	o = node->value;
	cbdata = checkdata;

	if (cbdata->check != o->ref)
		tpe_event_send(cbdata->tpe->event, "ObjectDelete", o,
				(void(*)(void*,void*))tpe_obj_cleanup, 
				cbdata->tpe);
	else if (o->changed){
		tpe_event_send(cbdata->tpe->event, 
				o->isnew ? "ObjectNew" : "ObjectChanged",
				o, tpe_event_nofree, NULL);
		o->changed = 0;
		o->isnew = 0;
	}
}

static void
tpe_obj_list_end(struct tpe *tpe){
	struct listcbdata data;

	data.check = tpe->obj->check;
	data.tpe = tpe;
 
	ecore_hash_for_each_node(tpe->obj->objhash, tpe_obj_list_cleanup_cb, 
			&data);
}

static void
tpe_obj_cleanup(struct tpe *tpe, struct object *o){
	int i;

	ecore_hash_remove(tpe->obj->objhash,(void*)o->oid);

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

	if (tpe->obj->home == o)
		tpe->obj->home = NULL;
	free(o);
}

/**
 * Check for home planet resource, and update 'home planet' special object.
 *
 */
static void
tpe_obj_home_check(struct tpe *tpe, struct object *o){
	uint32_t hptype;
	int i;

	assert(tpe); assert(tpe->obj); assert(o);
	assert(o->type == OBJTYPE_PLANET);
	hptype = tpe_resources_resourcedescription_get_by_name(tpe, 
				"Home Planet");

	if (hptype == (uint32_t)-1) return;

	for (i = 0 ; i < o->planet->nresources ; i ++){
		if (o->planet->resources[i].rid == hptype)
			break;
	}
	if (i == o->planet->nresources)
		/* Not it */
		return;

	/* Found the home planet */
	tpe->obj->home = o;
}

struct object *
tpe_obj_home_get(struct tpe *tpe){
	assert(tpe);
	assert(tpe->obj);
	return tpe->obj->home;
}

static void
tpe_obj_update_children(struct tpe *tpe, struct object *o, int noldchildren, int *oldchildren){
	struct object *child;
	struct tpe_obj *obj;
	int i,j;

	obj = tpe->obj;

	for (i = 0 ; i < noldchildren ; i ++){
		for (j = 0 ; j < o->nchildren ; j ++){
			if (oldchildren[i] == o->children[j])
				break;
		}
		if (j == o->nchildren){
			/* No longer a child */
			child = tpe_obj_obj_get_by_id(tpe,oldchildren[j]);
			if (!child){
				/* This should never happen */
				child = tpe_obj_obj_add(obj, oldchildren[j]);
				child->isnew = 1;
			} else {
				/* Don't mess up any which have already been
				 * updated */
				if (child->parent == o->oid){
					child->parent = TPE_OBJ_NIL;
					child->updated = 1;
				}
			}
		}
	}
	free(oldchildren);
	for (i = 0 ; i < o->nchildren ; i ++){
		child = tpe_obj_obj_get_by_id(tpe,o->children[i]);
		if (!child){
			child = tpe_obj_obj_add(obj,o->children[i]);
			child->updated = 1;
			child->isnew = 1;
			child->parent = o->oid;
		} else if (child->parent != o->oid){
			child->parent = o->oid;
			child->updated = 1;
		}
	}
}




static void 
tpe_obj_object_description_list_begin(struct tpe *tpe){

}
static void 
tpe_obj_object_description_list_end(struct tpe *tpe){

}
static uint64_t 
tpe_obj_object_description_updated(struct tpe *tpe, uint32_t odid){
	return 0;
}


