#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Edje.h>

#include "tpe.h"
#include "tpe_gui.h"
#include "tpe_comm.h"
#include "tpe_event.h"
#include "tpe_obj.h"
#include "tpe_ship.h"

struct tpe_gui {
	struct tpe *tpe;

	Ecore_Evas  *ee;
	Evas        *e;

	Evas_Object *main;

	Evas_Object *background;

	int zoom;

	/* Bounding box of the universe */
	struct {
		int64_t minx,miny;
		int64_t maxx,maxy;
	} bb;

	/* For mouse overs - may be more then one if they have a special
	 * delete */
	Evas_Object *popup;
};


struct tpe_gui_obj {
	Evas_Object *obj;

	struct tpe_gui *gui;
	struct object *object;

};

enum {
	WIDTH = 640,
	HEIGHT = 480,
	DEFAULT_ZOOM = 8388608,
};

#define URI	"localhost"	/* FIXME */

static void tpe_gui_edje_splash_connect(void *data, Evas_Object *o, 
		const char *emission, const char *source);

/* System event handlers */
static int tpe_gui_time_remaining(void *data, int eventid, void *event);
static int tpe_gui_object_update(void *data, int eventid, void *event);

/* Gui event handlers */
static void star_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event);
static void star_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event);
static void star_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event);

static const char *star_summary(struct tpe *tpe, struct object *object);

struct tpe_gui *
tpe_gui_init(struct tpe *tpe){
	struct tpe_gui *gui;

	gui = calloc(1,sizeof(struct tpe_gui));
	gui->tpe = tpe;
	gui->zoom = DEFAULT_ZOOM;
	
	/* FIXME: Should check for --nogui option */

	gui->ee = ecore_evas_software_x11_new(NULL, 0,  0, 0, WIDTH, HEIGHT);
	if (gui->ee == NULL) return 0;
	ecore_evas_title_set(gui->ee, "Thousand Parsec (E-Client)");
	ecore_evas_borderless_set(gui->ee, 0);
	ecore_evas_show(gui->ee);
	gui->e = ecore_evas_get(gui->ee);

	evas_font_path_append(gui->e, "data/");

	edje_init();

	gui->background = edje_object_add(gui->e);
	edje_object_file_set(gui->background,"edje/background.edj", 
				"Background");
	evas_object_move(gui->background, 0,0);
	evas_object_show(gui->background);
	evas_object_layer_set(gui->background,-100);
	evas_object_resize(gui->background,WIDTH,HEIGHT);

	gui->main = edje_object_add(gui->e);
	edje_object_file_set(gui->main,"edje/basic.edj", "Splash");
	evas_object_move(gui->main,0,0);
	evas_object_show(gui->main);
	evas_object_resize(gui->main,WIDTH,HEIGHT);
	edje_object_signal_callback_add(gui->main, "Splash.connect", "",
			tpe_gui_edje_splash_connect, tpe);


	/* Some code to display the time remaining */
	tpe_event_handler_add(gui->tpe->event, "MsgTimeRemaining",
			tpe_gui_time_remaining, gui);
	tpe_event_handler_add(gui->tpe->event, "ObjectNew",
			tpe_gui_object_update, gui);
	tpe_event_handler_add(gui->tpe->event, "ObjectChanged",
			tpe_gui_object_update, gui);

	return gui;
}


/**
 * Callback from the splash screen to start a connection
 */
static void 
tpe_gui_edje_splash_connect(void *data, Evas_Object *o, 
		const char *emission, const char *source){
	struct tpe *tpe;
	struct tpe_gui *gui;

	tpe = data;
	gui = tpe->gui;

	//tpe_comm_connect(tpe->comm, "localhost", 6923, "nash", "password");
	//tpe_comm_connect(tpe->comm, "10.0.0.1", 6923, "nash", "password");
	tpe_comm_connect(tpe->comm, "tranquillity.nash.id.au", 6923, "nash", "password");

	/* General errors */
/*	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_FAIL,
			tpe_fail, tpe);
*/
	/* Feature frames */
	/*
	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_AVAILABLE_FEATURES, 
			tpe_features, tpe);
	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_TIME_REMAINING, 
			tpe_time_remaining, tpe);
	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_LIST_OF_BOARDS, tpe_board, tpe);
	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_LIST_OF_RESOURCES_IDS,
			tpe_resource_description, tpe);
	*/	

	/* Create the map screen */
	evas_object_del(gui->main);
	gui->main = edje_object_add(gui->e);
	edje_object_file_set(gui->main,"edje/basic.edj", "Main");
	evas_object_move(gui->main,0,0);
	evas_object_resize(gui->main,WIDTH,HEIGHT);
	evas_object_show(gui->main);


	gui->popup = edje_object_add(gui->e);
	edje_object_file_set(gui->popup, "edje/basic.edj", "StarPopup");
	evas_object_resize(gui->popup,100,100);
	evas_object_layer_set(gui->popup, 1);
}


static int
tpe_gui_time_remaining(void *guip, int type, void *eventd){
	struct tpe_gui *gui;
	int *event;
	char buf[100];

	gui = guip;
	
	event = eventd;
	snprintf(buf,100,"%ds Remaining",ntohl(event[4]));

	edje_object_part_text_set(gui->main, "counter", buf);

	return 1;
}


static int 
tpe_gui_object_update(void *data, int eventid, void *event){
	struct tpe_gui *gui;
	struct object *obj;
	struct tpe_gui_obj *go;
	int changed = 0;
	Evas_Coord x,y;
	int width,height;
	
	gui = data;
	obj = event;

	if (obj->type == OBJTYPE_UNIVERSE || obj->type == OBJTYPE_GALAXY){
		return 1;
	}

	/* First update scaling factors */
	if (obj->pos.x > gui->bb.maxx) {changed ++;gui->bb.maxx = obj->pos.x;}
	if (obj->pos.y > gui->bb.maxy) {changed ++;gui->bb.maxy = obj->pos.y;}
	if (obj->pos.x < gui->bb.minx) {changed ++;gui->bb.minx = obj->pos.x;}
	if (obj->pos.y < gui->bb.miny) {changed ++;gui->bb.miny = obj->pos.y;}

	if (changed){
	}

	/* work out where to put it on screen */
	x = obj->pos.x / gui->zoom;
	y = obj->pos.y / gui->zoom;

	ecore_evas_geometry_get(gui->ee, 0,0,&width,&height);
	printf("Putting %s at %d,%d\n",obj->name, x + width / 2,y + height / 2);

	/* FIXME: Should allocate for:
	 * 	- Ships without parents
	 * 	- Systems 
	 */
	/* FIXME: Should be a nice function for this */
	if (obj->type == OBJTYPE_SYSTEM && obj->gui == NULL){
		printf("New obj\n");
		go = calloc(1,sizeof(struct tpe_gui_obj));
		go->object = obj;
		go->gui = gui;
		obj->gui = go;

		go->obj = edje_object_add(gui->e);
		edje_object_file_set(go->obj,"edje/basic.edj","Star");
		evas_object_resize(go->obj,8,8);
		evas_object_show(go->obj);
		evas_object_move(go->obj,x + width / 2,y + height / 2);

		edje_object_part_text_set(go->obj,"label", obj->name);

		evas_object_event_callback_add(go->obj,EVAS_CALLBACK_MOUSE_IN,
				star_mouse_in, go);
		evas_object_event_callback_add(go->obj,EVAS_CALLBACK_MOUSE_OUT,
				star_mouse_out, go);
		evas_object_event_callback_add(go->obj,EVAS_CALLBACK_MOUSE_DOWN,
				star_mouse_down, go);

	}

	if (obj->type == OBJTYPE_SYSTEM){
			
	} else if (obj->type == OBJTYPE_FLEET){

	}

	return 1;
}

static void
star_mouse_in(void *data, Evas *e, Evas_Object *eo, void *event){
	struct tpe_gui_obj *go;
	Evas_Coord x,y;
	Evas_Coord w,h,sw,sh;
	Evas_Coord px,py;

	go = data;

	/* FIXME: Layout correctly - always */
	edje_object_part_text_set(go->gui->popup, "text",
			star_summary(go->gui->tpe, go->object));

	evas_object_geometry_get(eo,&x,&y,NULL,NULL);
	evas_object_geometry_get(go->gui->popup,NULL,NULL, &w, &h);
	ecore_evas_geometry_get(go->gui->ee, NULL, NULL, &sw, &sh);

	px = x - w - 10;
	if (px < 0)
		px = x + 10;
	py = y - w / 2;
	if (py < 0)
		py = 2;
	else if (py + h > sh)
		py = sh - h - 5;
	

	evas_object_move(go->gui->popup,px,py);
	evas_object_show(go->gui->popup);

	


}

static const char *
star_summary(struct tpe *tpe, struct object *object){
	static char buf[BUFSIZ];
	int pos;
	int i,j,oid;
	struct object *child;

	pos = snprintf(buf,BUFSIZ,"<title>%s</title>",object->name);
	/* Planets first */
	for (i = 0 ; i < object->nchildren ; i ++){
		oid = object->children[i];
		child = tpe_obj_obj_get_by_id(tpe->obj, oid);
		if (!child) continue;
		if (child->type != OBJTYPE_PLANET) continue;
		pos += snprintf(buf + pos,BUFSIZ - pos, 
				"<planet>%s</planet>",child->name);
	}

	/* Now fleets */
	for (i = 0 ; i < object->nchildren ; i ++){
		oid = object->children[i];
		child = tpe_obj_obj_get_by_id(tpe->obj, oid);
		if (!child) continue;
		if (child->type != OBJTYPE_FLEET) continue;
		pos += snprintf(buf + pos,BUFSIZ - pos, 
				"<fleet>%s</fleet>",child->name);
		for (j = 0 ; j < child->fleet->nships ; j ++){
			pos += snprintf(buf + pos,BUFSIZ - pos,
				"<ship>%dx%s</ship>",
				child->fleet->ships[j].count,
				tpe_ship_design_name_get(tpe, 
					child->fleet->ships[j].design));
		}
	}

	return buf;	
}


static void
star_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event){
	struct tpe_gui_obj *go;

	go = data;
	printf("Mouse out\n");

	if (go->gui->popup){
		evas_object_hide(go->gui->popup);
	}
}

static void
star_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event){
	struct tpe_gui_obj *go;

	go = data;

	printf("Mouse down\n");

}
