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

} Smart_Data;

static const char classname[] = "Map";


static void smart_add(Evas_Object *);
static void smart_del(Evas_Object *);
static void smart_move(Evas_Object *, Evas_Coord x, Evas_Coord y);
static void smart_resize(Evas_Object *, Evas_Coord x, Evas_Coord h);

static const Evas_Smart_Class mapclass = {
	.name = classname,
	.version = EVAS_SMART_CLASS_VERSION,
	.add = smart_add,
	.del = smart_del,
	.move = smart_move,
	.resize = smart_resize,
};


Evas_Object *
gui_map_add(Evas *e){

	return NULL;
}

static void 
smart_add(Evas_Object *map){

}
static void 
smart_del(Evas_Object *map){

}
static void 
smart_move(Evas_Object *map, Evas_Coord x, Evas_Coord y){

}
static void 
smart_resize(Evas_Object *map, Evas_Coord w, Evas_Coord h){

}


