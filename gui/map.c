/**
 * Map widget for Galaxie.
 *
 * Draws a basic map for galaxie.  Automatically creates, destroys and
 * reparents stars and other widgets as objects are created. 
 *
 */ 


#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>

#include "../object_param.h"
#include "../tpe_event.h"

#include "map.h"
#include "star.h"
#include "widgetsupport.h"


typedef struct Smart_Data {
	Evas *e;

	objid_t id; /* Maps generally have an ID too */

	Evas_Object *clip;
	Evas_Object *bg;


} Smart_Data;

static const char maptype[] = "Map";


static void smart_add(Evas_Object *);
static void smart_del(Evas_Object *);
static void smart_move(Evas_Object *, Evas_Coord x, Evas_Coord y);
static void smart_resize(Evas_Object *, Evas_Coord x, Evas_Coord h);

static const Evas_Smart_Class mapclassdata = {
	.name = maptype,
	.version = EVAS_SMART_CLASS_VERSION,
	.add = smart_add,
	.del = smart_del,
	.move = smart_move,
	.resize = smart_resize,
};
static Evas_Smart *mapclass;


Evas_Object *
gui_map_add(Evas *e, objid_t id){
	Evas_Object *map;

	if (!e) return NULL;
	if (!mapclass) 
		mapclass = evas_smart_class_new(&mapclassdata);
	
	map = evas_object_smart_add(e, mapclass);

	if (id != 0xffffffff)
		gui_map_id_set(map,id);

	return map;
}

int
gui_map_id_set(Evas_Object *obj, objid_t id){
	Smart_Data *sd;

	sd = CHECK_TYPE_RETURN(obj, maptype, -1);
	sd->id = id;
	return 0;
}

static void 
smart_add(Evas_Object *map){
	Smart_Data *sd;

	sd = calloc(1,sizeof(struct Smart_Data));
	evas_object_smart_data_set(map, sd);

	sd = CHECK_TYPE(map, maptype);
	sd->e = evas_object_evas_get(map);

	sd->clip = evas_object_rectangle_add(sd->e);
	evas_object_show(sd->clip);
	evas_object_smart_member_add(sd->clip, map);

	sd->bg = evas_object_rectangle_add(sd->e);
	evas_object_color_set(sd->bg, 0,0,0,255);
	evas_object_show(sd->bg);
	evas_object_smart_member_add(sd->bg, map);
	evas_object_clip_set(sd->bg, sd->clip);

}

static void 
smart_del(Evas_Object *map){
	Smart_Data *sd;

	sd = CHECK_TYPE(map, maptype); 

	evas_object_del(sd->clip);
	evas_object_del(sd->bg);
}
static void 
smart_move(Evas_Object *map, Evas_Coord x, Evas_Coord y){
	Smart_Data *sd;

	sd = CHECK_TYPE(map, maptype);
	evas_object_move(sd->bg,x,y);
	evas_object_move(sd->clip,x,y);

}
static void 
smart_resize(Evas_Object *map, Evas_Coord w, Evas_Coord h){
	Smart_Data *sd;

	sd = CHECK_TYPE(map, maptype);
	evas_object_resize(sd->bg,w,h);
	evas_object_resize(sd->clip,w,h);
}


