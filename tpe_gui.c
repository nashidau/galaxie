#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Data.h>
#include <Evas.h>
#include <Edje.h>

#include "tpe.h"
#include "tpe_gui.h"
#include "tpe_board.h"
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

	struct {
		uint64_t zoom;
		int top,left;
		int w,h;
	} map;

	/* Bounding box of the universe */
	struct {
		int64_t minx,miny;
		int64_t maxx,maxy;
	} bb;

	/* For mouse overs - may be more then one if they have a special
	 * delete */
	Evas_Object *popup;

	Ecore_List *visible;


	/* FIXME: More then one board */
	Evas_Object *board;

	/* Window for message */
	Evas_Object *messagebox;
};

struct tpe_gui_obj {
	Evas_Object *obj;

	struct tpe_gui *gui;
	struct object *object;

	int nplanets;
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
static int tpe_gui_new_turn(void *data, int eventid, void *event);

static int tpe_gui_object_update(void *data, int eventid, void *event);
static int tpe_gui_object_delete(void *data, int eventid, void *event);



static int tpe_gui_board_update(void *data, int eventid, void *event);
static void board_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event);
static void board_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event);
static void board_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event);

/* Gui event handlers */
static void star_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event);
static void star_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event);
static void star_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event);
static void star_update(struct tpe *tpe, struct object *object);

static void fleet_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event);

static void map_key_down(void *data, Evas *e, Evas_Object *obj, void *event);

static void window_resize(Ecore_Evas *ee);
static void tpe_gui_redraw(struct tpe_gui *gui);


static const char *star_summary(struct tpe *tpe, struct object *object);

struct tpe_gui *
tpe_gui_init(struct tpe *tpe){
	struct tpe_gui *gui;

	gui = calloc(1,sizeof(struct tpe_gui));
	gui->tpe = tpe;
	gui->map.zoom = DEFAULT_ZOOM;
	gui->map.left = WIDTH / 2;
	gui->map.top = HEIGHT / 2;
	
	/* FIXME: Should check for --nogui option */

	gui->ee = ecore_evas_software_x11_new(NULL, 0,  0, 0, WIDTH, HEIGHT);
	if (gui->ee == NULL) {
		printf("Could not create ecore_evas_xll.\n");
		printf("Check you built evas and ecore with x11 support\n");
		return 0;
	}
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
	tpe_event_handler_add(gui->tpe->event, "ObjectDelete",
			tpe_gui_object_delete, gui);

	tpe_event_handler_add(gui->tpe->event, "BoardChanged",
			tpe_gui_board_update, gui);

	tpe_event_handler_add(gui->tpe->event, "NewTurn",
			tpe_gui_new_turn, gui);

	gui->visible = ecore_list_new();

	ecore_evas_data_set(gui->ee, "TPEGUI", gui);
	ecore_evas_callback_resize_set(gui->ee, window_resize);


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
	if (getenv("USER2")){
		printf("Pants connect!!!\n");
		tpe_comm_connect(tpe->comm, "10.0.0.1", 6923, "pants", "password");
	} else
	tpe_comm_connect(tpe->comm, "10.0.0.1", 6923, "nash", "password");
	//tpe_comm_connect(tpe->comm, "tranquillity.nash.id.au", 6923, "nash", "password");

	/* Create the map screen */
	evas_object_del(gui->main);
	gui->main = edje_object_add(gui->e);
	edje_object_file_set(gui->main,"edje/basic.edj", "Main");
	evas_object_move(gui->main,0,0);
	evas_object_resize(gui->main,WIDTH,HEIGHT);
	evas_object_show(gui->main);

	evas_object_event_callback_add(gui->main, EVAS_CALLBACK_KEY_DOWN,
				map_key_down, gui);
	evas_object_focus_set(gui->main, 1);

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
tpe_gui_new_turn(void *data, int eventid, void *event){
	struct tpe_gui *gui = data;
	char buf[200];

	snprintf(buf,200,"Thousand Parsec :: %s :: Turn %d", 
			gui->tpe->racename, gui->tpe->turn);
	ecore_evas_title_set(gui->ee,buf);

	return 1;
}


static int 
tpe_gui_object_update(void *data, int eventid, void *event){
	struct tpe_gui *gui;
	struct object *obj,*parent;
	struct tpe_gui_obj *go;
	Evas_Coord x,y;
	
	gui = data;
	obj = event;

	if (obj->type == OBJTYPE_UNIVERSE || obj->type == OBJTYPE_GALAXY){
		return 1;
	}

	/* work out where to put it on screen */
	x = obj->pos.x / gui->map.zoom;
	/* Y is flipped - it's cartesian */
	y = -obj->pos.y / gui->map.zoom;

	/* FIXME: Should be a nice function for this */
	if (obj->type == OBJTYPE_SYSTEM && obj->gui == NULL){
		go = calloc(1,sizeof(struct tpe_gui_obj));
		go->object = obj;
		go->gui = gui;
		obj->gui = go;

		go->obj = edje_object_add(gui->e);
		edje_object_file_set(go->obj,"edje/basic.edj","Star");
		evas_object_resize(go->obj,8,8);
		evas_object_show(go->obj);
		evas_object_move(go->obj,x + gui->map.left,y + gui->map.top);

		edje_object_part_text_set(go->obj,"label", obj->name);

		evas_object_event_callback_add(go->obj,EVAS_CALLBACK_MOUSE_IN,
				star_mouse_in, go);
		evas_object_event_callback_add(go->obj,EVAS_CALLBACK_MOUSE_OUT,
				star_mouse_out, go);
		evas_object_event_callback_add(go->obj,EVAS_CALLBACK_MOUSE_DOWN,
				star_mouse_down, go);

		ecore_list_append(gui->visible, obj);

	}
	if (obj->type == OBJTYPE_FLEET){
		parent = tpe_obj_obj_get_by_id(gui->tpe->obj, obj->parent);
		if (parent != NULL && (parent->type == OBJTYPE_SYSTEM ||
					parent->type == OBJTYPE_PLANET)){
			if (obj->gui){
				evas_object_del(obj->gui->obj);
				free(obj->gui);
				obj->gui = 0;
				/* Remove it from the list of visible items */
				if (ecore_list_goto(gui->visible, obj))
					ecore_list_remove(gui->visible);
			}
		} else {
	
			if (obj->gui == NULL){
				go = calloc(1,sizeof(struct tpe_gui_obj));
				go->object = obj;
				go->gui = gui;
				obj->gui = go;

				go->obj = edje_object_add(gui->e);
				evas_object_event_callback_add(go->obj,
						EVAS_CALLBACK_MOUSE_DOWN,
						fleet_mouse_down, obj);

				ecore_list_append(gui->visible, obj);

			}
			go = obj->gui;

			edje_object_file_set(go->obj,"edje/basic.edj","Fleet");
			evas_object_resize(go->obj,8,8);
			evas_object_show(go->obj);
			evas_object_move(go->obj,x + gui->map.left,
					y + gui->map.top);

		}
	}

	star_update(gui->tpe, obj);

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
	int i,j,k,oid;
	struct object *child, *gchild;

	pos = snprintf(buf,BUFSIZ,"<title>%s</title>",object->name);
	

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

	/* Planets first */
	for (k = 0 ; k < object->nchildren ; k ++){
		oid = object->children[k];
		child = tpe_obj_obj_get_by_id(tpe->obj, oid);
		if (!child) continue;
		if (child->type != OBJTYPE_PLANET) continue;
		pos += snprintf(buf + pos,BUFSIZ - pos, 
				"<planet>%s%s</planet>",child->name,
				(child->owner != -1) ? " *": "");
		for (i = 0 ; i < child->nchildren ; i ++){
			oid = child->children[i];
			gchild = tpe_obj_obj_get_by_id(tpe->obj, oid);
			if (!gchild) continue;
			if (gchild->type != OBJTYPE_FLEET) continue;
			pos += snprintf(buf + pos,BUFSIZ - pos, 
					"<fleet>%s</fleet>",gchild->name);
			for (j = 0 ; j < gchild->fleet->nships ; j ++){
				pos += snprintf(buf + pos,BUFSIZ - pos,
						"<ship>%dx%s</ship>",
						gchild->fleet->ships[j].count,
						tpe_ship_design_name_get(tpe, 
							gchild->fleet->ships[j].design));
			}
		}

	}

	return buf;	
}

/**
 * Updates the color and appearance of a star.
 *
 *
 *
 */
static void
star_update(struct tpe *tpe, struct object *object){
	struct object *child;
	int childid;
	int nchildren;
	int nowned, nother, nfriendly;
	const char *state = NULL;
	int i;

	if (object->type == OBJTYPE_PLANET){
		childid = object->parent;
		child = tpe_obj_obj_get_by_id(tpe->obj, childid);
		if (child == NULL) return;
		object = child;
	}

	if (object->type != OBJTYPE_SYSTEM)
		return;
	
	nchildren = nother = nowned = nfriendly = 0;

	for (i = 0 ; i < object->nchildren ; i ++){
		childid = object->children[i];
		child = tpe_obj_obj_get_by_id(tpe->obj, childid);
		if (child == NULL) continue;
		if (child->type != OBJTYPE_PLANET) continue;
		nchildren ++;
		if (child->owner == tpe->player) 
			nowned ++;
		else if (child->owner != -1)
			nother ++;
	}

	if (nowned > 0 && nother > 0)
		state = "Contested";
	else if (nother > 0)
		state = "Enemy";
	else if (nowned > 0 && nfriendly > 0)
		state = "Shared";
	else if (nfriendly > 0)
		state = "Friendly";
	else if (nowned > 0)
		state = "Own";
	
	/* FIXME: Should only emit if not in the right state */
	if (state){
		printf("Emit %s\n",state);
		edje_object_signal_emit(object->gui->obj, state, "app");
	}
	
}

/**
 * Event handler for object delete.
 *
 * Cleans up any GUI data for the object.
 *
 * @param date Gui pointer.
 * @param eventid Ecore event type.
 * @param event Object being deleted.
 * @return 1 Always (propogate event).
 */
static int 
tpe_gui_object_delete(void *data, int eventid, void *event){
	struct object *obj;
	struct tpe_gui_obj *go;

	obj = event;
	
	/* Nothing to do */
	if (obj->gui == NULL) return 1;

	go = obj->gui;

	ecore_list_goto(go->gui->visible, obj);
	ecore_list_remove(go->gui->visible);

	if (go->obj) evas_object_del(go->obj);
	memset(go, 0, sizeof(struct tpe_gui_obj));

	free(go);
	obj->gui = NULL;

	return 1;
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

	tpe_obj_obj_dump(go->object);
}
static void
fleet_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event){
	tpe_obj_obj_dump(data);

}

static void 
map_key_down(void *data, Evas *e, Evas_Object *obj, void *event){
	Evas_Event_Key_Down *key = event;
	struct tpe_gui *gui = data;

	if (strcmp(key->keyname, "equal") == 0)
		gui->map.zoom /= 2; /* FIXME: Sanity check */
	else if (strcmp(key->keyname, "minus") == 0)
		gui->map.zoom *= 2; /* FIXME: Sanity check */
	else if (strcmp(key->keyname, "Left") == 0)
		gui->map.left += 100;	
	else if (strcmp(key->keyname, "Right") == 0)
		gui->map.left -= 100;	
	else if (strcmp(key->keyname, "Up") == 0)
		gui->map.top += 100;	
	else if (strcmp(key->keyname, "Down") == 0)
		gui->map.top -= 100;	
	else if (strcmp(key->keyname, "Home") == 0){
		/* FIXME: Find homeworld, and center map */
	} else {
		return;
	}

	tpe_gui_redraw(gui);
}



static void 
window_resize(Ecore_Evas *ee){
	struct tpe_gui *gui;
	int neww, newh;
	int dw,dh;

	gui = ecore_evas_data_get(ee, "TPEGUI");
	if (gui == NULL) return;

	ecore_evas_geometry_get(ee, NULL, NULL, &neww, &newh);
	dw = neww - gui->map.w;
	dh = newh - gui->map.h;

	gui->map.left += dw / 2;
	gui->map.top += dh / 2;
	gui->map.w = neww;
	gui->map.h = newh;

	evas_object_resize(gui->main, gui->map.w, gui->map.h);

	/* FIXME: Move message icons */

	tpe_gui_redraw(gui);
}


static void
tpe_gui_redraw(struct tpe_gui *gui){
	struct object *obj;
	int x,y;

	ecore_list_goto_first(gui->visible);
	while ((obj = ecore_list_next(gui->visible))){
		x = obj->pos.x / gui->map.zoom;
		y = -obj->pos.y / gui->map.zoom;
		evas_object_move(obj->gui->obj,x + gui->map.left,y + gui->map.top);
	}

	//gui->redraw = 0;

}

/**
 * Handles for the GUI Board display 
 */

static int 
tpe_gui_board_update(void *data, int eventid, void *event){
	struct tpe_gui *gui = data;
	struct board_update *board = event;
	Evas_Object *o;
	char buf[20];

	if (gui->board == NULL){
		gui->board = o = edje_object_add(gui->e);
		evas_object_event_callback_add(o, 
				EVAS_CALLBACK_MOUSE_DOWN,
				board_mouse_down, gui);
		evas_object_event_callback_add(o, 
				EVAS_CALLBACK_MOUSE_IN,
				board_mouse_in, gui);
		evas_object_event_callback_add(o, 
				EVAS_CALLBACK_MOUSE_OUT,
				board_mouse_out, gui);

		edje_object_file_set(o,"edje/basic.edj","Board");
		evas_object_show(o);
		evas_object_move(o, 0,20); /* FIXME */
		evas_object_resize(o,20,20);
	}
	o = gui->board;

	snprintf(buf,20, "%d/%d",board->unread, board->messages);
	edje_object_part_text_set(o, "Text", buf);

	return 1;
}


static void board_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event){

}
static void board_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event){

}

/**
 * Handler for someone clicking on a MsgBoard Icon
 *
 * Opens on first unread message 
 */
static void 
board_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event){
	struct tpe_gui *gui = data;
	struct message *message;

	if (gui->messagebox == NULL){
		Evas_Object *o;
		gui->messagebox = o = edje_object_add(gui->e);
		edje_object_file_set(o, "edje/basic.edj", "MessageBox");
		evas_object_move(o, 20,20);
		evas_object_resize(o, 200,150);
	} 
	evas_object_show(gui->messagebox);
	/* FIXME: Check on screen */

	message = tpe_board_board_message_unread_get(gui->tpe, 1);

	/* FIXME: Handle no unread messages */
	message->unread = 0;

	edje_object_part_text_set(gui->messagebox, "Title", message->title);
	edje_object_part_text_set(gui->messagebox, "Body", message->body);

}


