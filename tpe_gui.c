#include <arpa/inet.h>
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Data.h>
#include <Ecore_Job.h>
#include <Evas.h>
#include <Edje.h>

//#include <Imlib2.h>	/* For screenshots */

#include "tpe.h"
#include "tpe_board.h"
#include "tpe_comm.h"
#include "tpe_event.h"
#include "tpe_obj.h"
#include "tpe_orders.h"
#include "tpe_ship.h"
#include "tpe_util.h" /* For struct reference */
#include "tpe_reference.h"
#include "tpe_resources.h"
#include "server.h"

#include "tpe_gui.h"
#include "tpe_gui_private.h"
#include "tpe_gui_orders.h"
#include "gui_window.h"
#include "gui_list.h"
#include "ewl/gewl_object.h"


enum {
	WIDTH = 640,
	HEIGHT = 480,
	DEFAULT_ZOOM = 1 << 23,

	LAYER_WINDOW = 10,
};

#define KEY_TPE_GUI	"TPEGUI"

static void gui_edje_splash_connect(void *data, Evas_Object *o, 
		const char *emission, const char *source);

/* System event handlers */
static int gui_time_remaining(void *data, int eventid, void *event);
static int gui_new_turn(void *data, int eventid, void *event);

static int gui_object_update(void *data, int eventid, void *event);
static int gui_object_delete(void *data, int eventid, void *event);

/* Connection events */
static int gui_connected_handler(void *data, int eventid, void *event);

/* Board gui events */
static int gui_board_update(void *data, int eventid, void *event);
static void board_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event);
static void board_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event);
static void board_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event);

/* Resource handlers */
static int gui_resource_new(void *data, int eventid, void *event);

/* Object window event handlers */
/* FIXME: objectwindow or object or objectbox - pick one */
static Evas_Object *gui_objectwindow_add(struct gui *gui);
static void gui_edje_object_change(void *data, Evas_Object *objectbox, 
		const char *emission, const char *source);
static void gui_objectbox_object_set(struct gui *gui, Evas_Object *objectbox, struct object *object);
static void gui_objectbox_clean(Evas_Object *objectbox);
static void gui_objectbox_ordersedit(void *gui, Evas_Object *orderbox, 
		const char *emission, const char *source);

/* Order window functions */

/* Message Window event handlers */
static Evas_Object *gui_messagebox_add(struct gui *gui);
static void gui_edje_message_change(void *data, Evas_Object *o, 
			const char *emission, const char *source);
static void gui_messagebox_message_set(struct gui *gui, 
			Evas_Object *messagebox, struct message *msg);
static void gui_messagebox_ref_free(Evas_Object *messagebox);

/* Gui event handlers */
static void star_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event);
static void star_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event);
static void star_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event);
static void star_update(struct tpe *tpe, struct object *object);

static void fleet_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event);

static void map_key_down(void *data, Evas *e, Evas_Object *obj, void *event);

static void window_resize(Ecore_Evas *ee);
static void gui_redraw(struct gui *gui);

Evas_Object *gui_ref_object_get(struct gui *gui, struct reference *ref);
Evas_Object *gui_object_icon_get(struct gui *gui, uint32_t oid, int active);
static void reference_object_show(void *idv, Evas *e, Evas_Object *eo, void *event);
static void gui_icon_del_cb(Evas_Object *icon);

static int gui_screengrab(struct gui *gui);
static int gui_toggle_names(struct gui *gui);


static const char *star_summary(struct tpe *tpe, struct object *object);

struct gui *
gui_init(struct tpe *tpe, const char *theme, unsigned int fullscreen){
	struct gui *gui;

	gui = calloc(1,sizeof(struct gui));
	gui->tpe = tpe;
	gui->map.zoom = DEFAULT_ZOOM;
	gui->map.left = WIDTH / 2;
	gui->map.top = HEIGHT / 2;
	gui->textvisible = 1;

	/* FIXME: need to be a bit more dynamic here */
	gui->ee = ecore_evas_software_x11_new(NULL, 0,  0, 0, WIDTH, HEIGHT);
	if (gui->ee == NULL) {
		printf("Could not create ecore_evas_xll.\n");
		printf("Check you built evas and ecore with x11 support\n");
		return NULL;
	}
	gui->screenw = WIDTH;
	gui->screenh = HEIGHT;
	ecore_evas_title_set(gui->ee, "GalaxiE");
	ecore_evas_borderless_set(gui->ee, 0);
	ecore_evas_show(gui->ee);
	if (fullscreen)
		ecore_evas_fullscreen_set(gui->ee, 1);
	gui->e = ecore_evas_get(gui->ee);

	evas_font_path_append(gui->e, 
			"/usr/share/fonts/truetype/ttf-bitstream-vera");

	edje_init();

	gui->background = edje_object_add(gui->e);
	edje_object_file_set(gui->background,"edje/background.edj", 
				"Background");
	evas_object_move(gui->background, 0,0);
	evas_object_show(gui->background);
	evas_object_layer_set(gui->background,-100);
	if (fullscreen){
		Evas_Coord w,h;
		ecore_evas_geometry_get(gui->ee, NULL, NULL, &w, &h);
		evas_object_resize(gui->background, w, h);
	} else 
		evas_object_resize(gui->background,WIDTH,HEIGHT);

	gui->main = edje_object_add(gui->e);
	edje_object_file_set(gui->main,"edje/basic.edj", "Splash");
	evas_object_move(gui->main,0,0);
	evas_object_show(gui->main);
	evas_object_resize(gui->main,WIDTH,HEIGHT);
	edje_object_signal_callback_add(gui->main, "Splash.connect", "",
			gui_edje_splash_connect, tpe);


	/* Some code to display the time remaining */
	tpe_event_handler_add(gui->tpe->event, "MsgTimeRemaining",
			gui_time_remaining, gui);

	tpe_event_handler_add(gui->tpe->event, "ObjectNew",
			gui_object_update, gui);
	tpe_event_handler_add(gui->tpe->event, "ObjectChanged",
			gui_object_update, gui);
	tpe_event_handler_add(gui->tpe->event, "ObjectDelete",
			gui_object_delete, gui);

	tpe_event_handler_add(gui->tpe->event, "BoardUpdate",
			gui_board_update, gui);

	tpe_event_handler_add(gui->tpe->event, "Connected",
			gui_connected_handler, gui);

	tpe_event_handler_add(gui->tpe->event, "NewTurn",
			gui_new_turn, gui);

	tpe_event_handler_add(gui->tpe->event, "ResourceNew",
			gui_resource_new, gui);

	gui->visible = ecore_list_new();
	gui->boards = ecore_list_new();

	ecore_evas_data_set(gui->ee, "TPEGUI", gui);
	ecore_evas_callback_resize_set(gui->ee, window_resize);


	return gui;
}


/**
 * Callback from the splash screen to start a connection
 */
static void 
gui_edje_splash_connect(void *data, Evas_Object *o, 
		const char *emission, const char *source){
	struct tpe *tpe;
	struct gui *gui;

	tpe = data;
	gui = tpe->gui;

	/* FIXME */
	//tpe_comm_connect(tpe->comm, "tranquillity.nash.id.au", 6923, "default", "nash", "password");
}


static int
gui_connected_handler(void *data, int eventid, void *event){
	struct gui *gui;

	gui = data;

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

	gui->boardpopup = edje_object_add(gui->e);
	edje_object_file_set(gui->boardpopup,"edje/basic.edj", "BoardPopup");
	evas_object_resize(gui->boardpopup, 200,200);
	evas_object_layer_set(gui->boardpopup, 1);

	return 1;
}


static int
gui_time_remaining(void *guip, int type, void *eventd){
	struct gui *gui;
	struct msg *msg;
	char buf[100];
	int rem;

	gui = guip;

	msg = eventd;
	if (tpe_util_parse_packet(msg->data,msg->end,"i", &rem) == 1){
		snprintf(buf,100,"%ds Remaining",rem);
	} else {
		snprintf(buf,100,"? Remaining");
	}
	edje_object_part_text_set(gui->main, "counter", buf);

	return 1;
}



static int 
gui_new_turn(void *data, int eventid, void *event){
	struct gui *gui = data;
	struct msg *msg = event;
	char buf[200];

	if (gui->server != msg->server) return 1;

	snprintf(buf,200,"GalaxiE :: %s :: Turn %d", 
			gui->tpe->racename, gui->tpe->turn);
	ecore_evas_title_set(gui->ee,buf);

	if (gui->record){
		/* Give it 5 seconds for dumping */
		ecore_timer_add(5,(int(*)(void*))gui_screengrab,gui);
	}


	return 1;
}


static int 
gui_object_update(void *data, int eventid, void *event){
	struct gui *gui;
	struct object *obj,*parent;
	struct gui_obj *go;
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
		go = calloc(1,sizeof(struct gui_obj));
		go->object = obj;
		go->gui = gui;
		obj->gui = go;

		go->obj = edje_object_add(gui->e);
		edje_object_file_set(go->obj,"edje/basic.edj","Star");
		evas_object_resize(go->obj,8,8);
		evas_object_show(go->obj);
		evas_object_move(go->obj,x + gui->map.left,y + gui->map.top);
		if (!gui->textvisible) 
			edje_object_signal_emit(go->obj, "HideText", "app");

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
		parent = tpe_obj_obj_get_by_id(gui->tpe, obj->parent);
		if (parent != NULL && (parent->type == OBJTYPE_SYSTEM ||
					parent->type == OBJTYPE_PLANET)){
			if (obj->gui){
				evas_object_del(obj->gui->obj);
				free(obj->gui);
				obj->gui = NULL;
				/* Remove it from the list of visible items */
				if (ecore_list_goto(gui->visible, obj))
					ecore_list_remove(gui->visible);
			}
		} else {
			/* FIXME: Move to generic function */	
			if (obj->gui == NULL){
				go = calloc(1,sizeof(struct gui_obj));
				go->object = obj;
				go->gui = gui;
				obj->gui = go;

				go->obj = edje_object_add(gui->e);
				evas_object_event_callback_add(go->obj,
						EVAS_CALLBACK_MOUSE_DOWN,
						fleet_mouse_down, go);

				ecore_list_append(gui->visible, obj);

			}
			go = obj->gui;

			edje_object_file_set(go->obj,"edje/basic.edj","Fleet");
			evas_object_resize(go->obj,8,8);
			evas_object_show(go->obj);
			evas_object_move(go->obj,x + gui->map.left,
					y + gui->map.top);
			edje_object_part_text_set(go->obj, "Name", obj->name);
			if (!gui->textvisible) 
				edje_object_signal_emit(go->obj, "HideText", 
						"app");
		}
	} else 
		star_update(gui->tpe, obj);

	return 1;
}

static void
star_mouse_in(void *data, Evas *e, Evas_Object *eo, void *event){
	struct gui_obj *go;
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
		py = sh - h - 5;;
	

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
		child = tpe_obj_obj_get_by_id(tpe, oid);
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
		child = tpe_obj_obj_get_by_id(tpe, oid);
		if (!child) continue;
		if (child->type != OBJTYPE_PLANET) continue;
		pos += snprintf(buf + pos,BUFSIZ - pos, 
				"<planet>%s%s</planet>",child->name,
				(child->owner != -1) ? " *": "");
		for (i = 0 ; i < child->nchildren ; i ++){
			oid = child->children[i];
			gchild = tpe_obj_obj_get_by_id(tpe, oid);
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
	struct object *child,*parent;
	int childid,parentid;
	int nchildren;
	int nowned, nother, nfriendly;
	const char *state = NULL;
	int i;

	/* If it is a planet - move to the parent */
	if (object->type == OBJTYPE_PLANET){
		parentid = object->parent;
		if (parentid == TPE_OBJ_NIL) return;
		parent = tpe_obj_obj_get_by_id(tpe, parentid);
		/* Check for a weird universe */
		if (parent) {
			if (parent->type != OBJTYPE_SYSTEM){
				printf("Warning: Planet with unusual parent\n");
				tpe_obj_obj_dump(parent);
				return;
			}
		}
		/* Update later */
		if (parent == NULL || parent->gui == NULL) return;
		object = parent;
	}

	if (object->type != OBJTYPE_SYSTEM)
		return;
	if (object->gui == NULL) return;

	assert(object->gui);

	nchildren = nother = nowned = nfriendly = 0;
	
	/* FIXME: Need to check for alliances */
	for (i = 0 ; i < object->nchildren ; i ++){
		childid = object->children[i];
		child = tpe_obj_obj_get_by_id(tpe, childid);
		if (child == NULL) continue;
		if (child->type != OBJTYPE_PLANET) continue;
		nchildren ++;
		if (child->owner == tpe->player) 
			nowned ++;
		else if (child->owner != 0)
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
	
	if (state && object->gui->state != state){
		object->gui->state = state;
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
/* FIXME: Need to delete any object references to this object 
 * 	For instance in a object window */
static int 
gui_object_delete(void *data, int eventid, void *event){
	struct object *obj;
	struct gui_obj *go;

	obj = event;

	/* Nothing to do */
	if (obj->gui == NULL) return 1;

	go = obj->gui;

	if (go->info) evas_object_del(go->info);
	if (go->orders) evas_object_del(go->orders);

	if (ecore_list_goto(go->gui->visible, obj))
		ecore_list_remove(go->gui->visible);

	if (go->obj) evas_object_del(go->obj);

	memset(go, 0, sizeof(struct gui_obj));

	free(go);
	obj->gui = NULL;

	return 1;
}





static void
star_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event){
	struct gui_obj *go;

	go = data;

	if (go->gui->popup){
		evas_object_hide(go->gui->popup);
	}
}

/** 
 * Handler for mouse down on stars. 
 *
 * Pops a window for the star.  A right click pops up a menu, to allow
 * selection of planet or fleet.
 *
 * Left click - select the star.
 * Right click - menu.
 *
 * @param data Gui object data for star.
 * @param e Evas.
 * @param obj The star object .
 * @param event The evas mouse event.
 **/
static void
star_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event){
	Evas_Object *o;
	//Evas_Event_Mouse_Down *mouse = event;
	struct gui_obj *go = data;

	if (go->info){
		evas_object_raise(go->info);
	} else {
/*
		o = gui_objectwindow_add(go->gui);

		gui_objectbox_object_set(go->gui, o, go->object);

		evas_object_show(o);
*/
//		o = tpe_ewl_planet_add(go->gui, go->object);
		go->info = o;
	}

}

static void
test_orders_show(void *ob, Evas *e,Evas_Object *tb, void *mouse){
	Evas_Event_Mouse_Down *down = mouse;
	Evas_Coord x,y;
	Evas_Textblock_Cursor *cur;

	printf("Got mouse down: %d %d\n",down->canvas.x,down->canvas.y);
	printf("              : %d %d\n",down->output.x,down->output.y);
	evas_object_geometry_get(tb,&x,&y,NULL,NULL);
	x = down->canvas.x - x;
	y = down->canvas.y - y;

	cur = evas_object_textblock_cursor_new(tb);
	evas_textblock_cursor_char_coord_set(cur, x,y);

	printf("Node text is %s\n", evas_textblock_cursor_node_text_get(cur));
	printf("Node is %s\n", evas_textblock_cursor_node_format_get(cur));
	printf("Pos is  %d\n", evas_textblock_cursor_pos_get(cur));
/*
evas_textblock_cursor_char_geometry_get
evas_textblock_cursor_char_geometry_get
evas_textblock_cursor_node_format_get
evas_textblock_cursor_node_text_get
evas_textblock_cursor_pos_get*/
}


/**
 * Creates a new message window.
 *
 */
static Evas_Object *
gui_objectwindow_add(struct gui *gui){
	Evas_Object *o;
	Evas_Object *tb;

	o = edje_object_add(gui->e);
	edje_object_file_set(o,"edje/basic.edj", "ObjectInfo");
	evas_object_resize(o, 388, 419);
	gui_window_add(gui, o);

	edje_object_signal_callback_add(o,
			"mouse,clicked,*", "Parent", 
			gui_edje_object_change, gui);
	edje_object_signal_callback_add(o,
			"mouse,clicked,*", "EditOrders",
			gui_objectbox_ordersedit, gui);

	evas_object_event_callback_add(o, EVAS_CALLBACK_FREE,
			(void*)gui_objectbox_clean, o);

	tb = edje_object_part_object_get(o,"Orders");
	if (tb){
		evas_object_event_callback_add(tb, EVAS_CALLBACK_MOUSE_DOWN,
			(void*)test_orders_show,o);
	}

	return o;
}


/**
 * Mouse click handler for mouse clicks in the Object view window.
 *
 */
static void
gui_edje_object_change(void *data, Evas_Object *objectbox, 
		const char *emission, const char *source){
	struct gui *gui = data;
	struct object *obj,*target;

	obj = evas_object_data_get(objectbox, "Object");
	assert(obj);
	if (obj == NULL) return;

	if (strcmp(source, "Child") == 0){
		target = tpe_obj_obj_child_get(gui->tpe, obj);
	} else if (strcmp(source, "Parent") == 0){
		target = tpe_obj_obj_parent_get(gui->tpe, obj);
	} else if (strcmp(source, "Next") == 0){
		target = tpe_obj_obj_sibling_get(gui->tpe, obj, 1);
	} else {
		target = tpe_obj_obj_sibling_get(gui->tpe, obj, 0);
	}

	if (target)
		gui_objectbox_object_set(gui, objectbox, target);
}

static void
gui_objectbox_object_set(struct gui *gui, Evas_Object *objectbox,
		struct object *object){
	Evas_Object *icon;
	const char *orderstr;
	char buf[50];
	int i,nchild;

	gui_objectbox_clean(objectbox);

	edje_object_part_text_set(objectbox, "Name", object->name);
	
	/* FIXME: Get the player name */
	if (object->owner == -1)
		edje_object_part_text_set(objectbox, "Owner", "Unowned");
	else if (object->owner == gui->tpe->player)
		edje_object_part_text_set(objectbox, "Owner", "Mine");
	else {
		snprintf(buf,50,"Other (%d)", object->owner);
		edje_object_part_text_set(objectbox, "Owner", buf);
	}

	orderstr = tpe_orders_str_get(gui->tpe, object);
	edje_object_part_text_set(objectbox, "Orders", orderstr);

	if (object->nordertypes)
		/* Some orders are possible - set the orders active */
		edje_object_signal_emit(objectbox, "EditOrders", "app");
	else
		/* None, set the state off */
		edje_object_signal_emit(objectbox, "NoEditOrders", "app");

	evas_object_data_set(objectbox, "Object", object);

	/* Get the icon for the object */
	icon = gui_object_icon_get(gui, object->oid, 0);
	if (icon) edje_object_part_swallow(objectbox, "icon", icon);

	icon = gui_object_icon_get(gui, object->parent, 1);
	if (icon) edje_object_part_swallow(objectbox, "parent", icon);

	/* Children */
	for (i = 0, nchild = 0 ; i < object->nchildren && nchild < 5 ; i ++){
		icon = gui_object_icon_get(gui, object->children[i], 1);
		if (icon) {
			snprintf(buf,50,"Child%02d", nchild);
			edje_object_part_swallow(objectbox, buf, icon);
			nchild ++;
		}
	}
}

static void
gui_objectbox_clean(Evas_Object *objectbox){
	struct object *object;
	Evas_Object *obj;
	char buf[50];
	int i;

	object = evas_object_data_get(objectbox, "Object");
	if (object){
		assert(object->gui);
		assert(object->gui->info == objectbox);
		object->gui->info = NULL;
	}

	evas_object_data_set(objectbox, "Object", NULL);

	obj = edje_object_part_swallow_get(objectbox, "icon");
	if (obj){
		edje_object_part_unswallow(objectbox, obj);
		evas_object_del(obj);
	}
	obj = edje_object_part_swallow_get(objectbox, "parent");
	if (obj){
		edje_object_part_unswallow(objectbox, obj);
		evas_object_del(obj);
	}

	for (i = 0 ; i < 5 ; i ++){
		snprintf(buf,50,"Child%02d",i);
		obj = edje_object_part_swallow_get(objectbox, buf);
		if (obj){
			edje_object_part_unswallow(objectbox, obj);
			evas_object_del(obj);

		}
	}

}


/**
 * Callback for when someone clicks the 'edit orders' button.
 */
static void
gui_objectbox_ordersedit(void *data, Evas_Object *objectbox, 
		const char *emission, const char *source){
	struct object *object;
	struct gui *gui = data;

	object = evas_object_data_get(objectbox, "Object");
	if (object == NULL) {
		fprintf(stderr,"Could not find object for orderbox!\n");
		return;
	}

	if (object->nordertypes < 1) return;

	if (gui_orders_edit(gui, object) != 0){
		assert(!"Error opening order window\n");
		fprintf(stderr,"Error opening order window\n");
	}
}



static void
fleet_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event){
	star_mouse_down(data,e,obj,event);

}

static void 
map_key_down(void *data, Evas *e, Evas_Object *obj, void *event){
	Evas_Event_Key_Down *key = event;
	struct gui *gui = data;
	struct object *home;

	/* FIXME: Need to recenter as I zoom in */
	if (strcmp(key->keyname, "equal") == 0){
		if (gui->map.zoom > 4)
			gui->map.zoom >>= 1;
	} else if (strcmp(key->keyname, "minus") == 0){
		if (gui->map.zoom < (1LL << 60))
			gui->map.zoom <<= 1;
	} else if (strcmp(key->keyname, "Left") == 0)
		gui->map.left += 100;	
	else if (strcmp(key->keyname, "Right") == 0)
		gui->map.left -= 100;	
	else if (strcmp(key->keyname, "Up") == 0)
		gui->map.top += 100;	
	else if (strcmp(key->keyname, "Down") == 0)
		gui->map.top -= 100;	
	else if (strcmp(key->keyname, "Home") == 0){
		
		/* FIXME: Find homeworld, and center map */
		home = tpe_obj_home_get(gui->tpe);
		if (home)	
			printf("Home is %s\n",home->name);
		else 
			printf("no home\n");
	} else if (strcmp(key->keyname, "F10") == 0){
		gui_toggle_names(gui);
	} else if (strcmp(key->keyname, "F11") == 0){
		ecore_evas_fullscreen_set(gui->ee,
				!ecore_evas_fullscreen_get(gui->ee));
	} else if (strcmp(key->keyname, "F2") == 0){
		printf("Planet list called\n");
		gui_list_planet_add(gui);
	} else if (strcmp(key->keyname,"F5") == 0){
		struct object *obj;
		printf("List of visible objects\n");
		ecore_list_first_goto(gui->visible);
		while ((obj = ecore_list_next(gui->visible))){
			printf("\t%s [%d] %llx %llx %llx,(Parent [%d])\n",
				obj->name,obj->oid,
				obj->pos.x,obj->pos.y, obj->pos.z,
				obj->parent);
		}
		
	} else if (strcmp(key->keyname, "Print") == 0){
		if (evas_key_modifier_is_set(
				evas_key_modifier_get(e), "Shift")){
			gui->record = !gui->record;
		} else 
			gui_screengrab(gui);

	} else {
		return;
	}

	gui_redraw(gui);
}



static void 
window_resize(Ecore_Evas *ee){
	struct gui *gui;
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

	gui_redraw(gui);
}


static void
gui_redraw(struct gui *gui){
	struct object *obj;
	int x,y;

	ecore_list_first_goto(gui->visible);
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
gui_board_update(void *data, int eventid, void *event){
	struct gui *gui = data;
	struct board_update *update = event;
	struct gui_board *board;
	Evas_Object *o;
	char buf[20];

	board = ecore_list_first_goto(gui->boards);
	while ((board = ecore_list_next(gui->boards))){
		if (board->boardid == update->id)
			break;
	}

	if (board == NULL){
		board = calloc(1,sizeof(struct gui_board));
		o = edje_object_add(gui->e);
		board->obj = o;
		board->boardid = update->id;
		board->gui = gui;
		ecore_list_append(gui->boards, board);

		evas_object_event_callback_add(o, 
				EVAS_CALLBACK_MOUSE_DOWN,
				board_mouse_down, board);
		evas_object_event_callback_add(o, 
				EVAS_CALLBACK_MOUSE_IN,
				board_mouse_in, board);
		evas_object_event_callback_add(o, 
				EVAS_CALLBACK_MOUSE_OUT,
				board_mouse_out, board);

		edje_object_file_set(o,"edje/basic.edj","Board");
		evas_object_show(o);
		/* FIXME: position not on top of other boards... */
		evas_object_move(o, 0,20); /* FIXME: Want this on the right */
		evas_object_resize(o,50,32);
		board->state = BOARD_READ;	

		if (update->name)
			board->name = strdup(update->name);
		if (update->desc)
			board->desc = strdup(update->desc);
	}
	o = board->obj;

	snprintf(buf,20, "%d/%d",update->unread, update->messages);
	edje_object_part_text_set(o, "Text", buf);

	if (update->unread == 0){
		if (board->state == BOARD_UNREAD){
			edje_object_signal_emit(o, "AllMessagesRead", "app");
			board->state = BOARD_READ;
		}
	} else {
		if (board->state == BOARD_READ){
			edje_object_signal_emit(o, "NewMessages", "app");
			board->state = BOARD_UNREAD;
		}
	}

	return 1;
}


static void 
board_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event){
	struct gui_board *board = data;
	struct gui *gui;
	Evas_Coord x,y,w,h,pw,ph,sw,sh,px,py;

	assert(board);
	
	gui = board->gui;

	edje_object_part_text_set(gui->boardpopup, "BoardTitle", board->name);
	edje_object_part_text_set(gui->boardpopup, "BoardDescription", 
			board->desc);

	evas_object_geometry_get(gui->boardpopup,NULL,NULL,&pw,&ph);
	evas_object_geometry_get(board->obj,&x,&y,&w,&h);
	ecore_evas_geometry_get(gui->ee, NULL,NULL,&sw,&sh);

	px = x + w;
	py = y + (h / 2) - (ph / 2);
	if (px + w > sw)
		px = x - pw;
	if (py < 0)
		py = 0;
	if (py > sh)
		py = sh - ph;

	evas_object_move(gui->boardpopup, px, py);
	evas_object_show(gui->boardpopup);


}
static void 
board_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event){
	struct gui_board *board = data;
	evas_object_hide(board->gui->boardpopup);
}

/**
 * Handler for someone clicking on a MsgBoard Icon
 *
 * Opens on first unread message 
 *
 * @param data Is board pointer
 * @param e Evas pointer
 * @param obj Object
 * @param event Mouse Down Event data
 */
static void 
board_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event){
	struct gui_board *board = data;
	struct message *message;
	Evas_Object *msgbox;

	msgbox = gui_messagebox_add(board->gui);

	/* FIXME: Check on screen */

	message = tpe_board_board_message_unread_get(board->gui->tpe, 
			board->boardid);
	if (message == NULL){
		message = tpe_board_board_message_turn_get(board->gui->tpe,
				board->boardid);
	}
	
	if (message == NULL){
		evas_object_del(msgbox);
		return;
	}

	gui_messagebox_message_set(board->gui, msgbox, message);
}

/**
 * Create a new mesage box.
 *
 * There can an unlimited number of message boxes at the moment.  
 *
 * This may need to be fixed - in particular, clicking on the board icon on
 * the left shouldn't keep adding new ones.  Left clicks anyway.
 *
 * @param gui TPE Gui pointer
 * @return New messagebox, or NULL on error.
 */
static Evas_Object *
gui_messagebox_add(struct gui *gui){
	Evas_Object *o;
	
	o = edje_object_add(gui->e);
	edje_object_file_set(o, "edje/basic.edj", "MessageBox");
	evas_object_resize(o, 289,304);
	gui_window_add(gui, o);
	
	edje_object_signal_callback_add(o,
			"mouse,clicked,*", "Next", 
			gui_edje_message_change, gui);
	edje_object_signal_callback_add(o,
			"mouse,clicked,*", "Prev", 
			gui_edje_message_change, gui);

	evas_object_event_callback_add(o,
			EVAS_CALLBACK_FREE, 
			(void*)gui_messagebox_ref_free, o);

	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
			(void*)evas_object_raise, o);

	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
			(void*)evas_object_raise, o);

	return o;
}

static void
gui_messagebox_ref_free(Evas_Object *messagebox){
	Evas_Object *obj;
	Evas_Object *icon;
	char buf[100];
	int i;

	/* Remove old swallowed objects */
	for (i = 1 ; i < 4 ; i ++){
		snprintf(buf,100,"Reference%d",i);
		obj = edje_object_part_swallow_get(messagebox, buf);
		if (obj == NULL) break;
		edje_object_part_unswallow(messagebox, obj);
		icon = edje_object_part_swallow_get(obj, "object");
		evas_object_del(obj);
		if (icon) evas_object_del(icon);
	}
}



/**
 * Sets the message in a message box.
 *
 * @param gui TPE Gui structure.
 * @param messagebox The messagebox object to set the message in.
 * @param msg The message to set.
 */
static void
gui_messagebox_message_set(struct gui *gui, 
		Evas_Object *messagebox, struct message *msg){
	Evas_Object *obj;
	char buf[100];
	int i;
	int ref;

	if (gui == NULL || messagebox == NULL) return;
	if (msg == NULL) return;

	gui_messagebox_ref_free(messagebox);

	snprintf(buf,100,"Message: %d  Turn: %d", msg->slot + 1, msg->turn);
	edje_object_part_text_set(messagebox, "MessageNumber", buf);

	edje_object_part_text_set(messagebox, "Title", msg->title);
	edje_object_part_text_set(messagebox, "Body", msg->body);
		
	tpe_board_board_message_read(gui->tpe, msg);

	evas_object_data_set(messagebox, "Message", msg);

	ref = 0;
	for (i = 0 ; i < msg->nrefs ; i ++){
		obj = gui_ref_object_get(gui, &msg->references[i]);
		if (obj == NULL){
			printf("No object for reference %d/%d\n",
					msg->references[i].type,
					msg->references[i].value);
			continue;
		}
		/* FIXME: Handle gracefully */
		if (ref >= 3)
			break;
		ref ++;
		snprintf(buf,100,"Reference%d",ref);
		edje_object_part_swallow(messagebox, buf, obj);
	}
}


/**
 * Mouse-click handler for mouse clicks in a message window.
 *
 * Left button: Do that operation (Next, Prev etc);
 * Middle button: Spawn a new window, and perform op in that window
 * Right button: (Next): Jump to next turn
 * 		Jump to first message in previous turn
 * 	
 * @param data GUI structuture
 * @param o The edje object
 * @param emission Signal name (mouse,click,N where N == button)
 * @param source "next" or "prev"
 */
static void
gui_edje_message_change(void *data, Evas_Object *o, const char *emission,
		const char *source){
	struct gui *gui = data;
	struct message *msg;
	int button;

	msg = evas_object_data_get(o, "Message");
	assert(msg);
	if (msg == NULL) return;

	if (strstr(emission, "2")){
		o = gui_messagebox_add(gui);
		button = 1;
	}

	/* FIXME: This is ugly... 3, 2 etc. */
	if (strstr(emission,"3"))
		button = 2;
	else 
		button = 1;
	


	if (strcmp(source,"Next") == 0){
		if (button == 1)
			msg = tpe_board_board_message_next(gui->tpe, msg);	
		else 
			msg = tpe_board_board_message_next_turn(gui->tpe, msg);	
	} else {
		/* Previous */
		if (button == 1)
			msg = tpe_board_board_message_prev(gui->tpe, msg);	
		else 
			msg = tpe_board_board_message_prev_turn(gui->tpe, msg);	
	}

	if (msg)
		gui_messagebox_message_set(gui, o, msg);

	return;
}


/** ------------------------------
 * TPE GUI Reference Functions
 */
/**
 * Allocates a new reference object for the given reference.
 *
 * For Objects it gets their image, and sets up even handlers for 
 */
Evas_Object *
gui_ref_object_get(struct gui *gui, struct reference *ref){

	assert(gui);
	assert(ref);
	if (gui == NULL || ref == NULL) return NULL;

	switch (ref->type){
	case REFTYPE_OBJECT:
		return gui_object_icon_get(gui,ref->value,1);
		break;
	default:
		printf("Unknown ref %d\n",ref->type);
	}
	return NULL;
}

/**
 * Gets a visual representation for an object.
 *
 * Returns it as an Evas_Image.
 *
 * If @ref 'active' is set, will return an object that responds to mouse overs
 * and mouse downs.
 *
 */
Evas_Object *
gui_object_icon_get(struct gui *gui, uint32_t oid, int active){
	Evas_Object *icon;
	Evas_Object *eo;
	struct object *obj;
	assert(gui);
	
	if (gui == NULL) return NULL;
	
	obj = tpe_obj_obj_get_by_id(gui->tpe, oid);
	if (obj == NULL){
		printf("Object icon get: Could not find %d\n",oid);
		return NULL;
	}

	eo = edje_object_add(gui->e);
	edje_object_file_set(eo, "edje/basic.edj", "ReferenceObject");
	evas_object_show(eo);
	evas_object_resize(eo,64,64);

	icon = evas_object_image_add(gui->e);
	if (icon == NULL) return NULL;

	/* FIXME: need to use media */
	switch(obj->type){
	case OBJTYPE_UNIVERSE:
		evas_object_image_file_set(icon, "edje/images/universe.png",NULL);
		break;
	case OBJTYPE_GALAXY:
		evas_object_image_file_set(icon, "edje/images/galaxy64.png",NULL);
		break;
	case OBJTYPE_SYSTEM:
		evas_object_image_file_set(icon, "edje/images/star.png",NULL);
		break;
	case OBJTYPE_PLANET:
		evas_object_image_file_set(icon, 
			"edje/images/planet.png", NULL);
		break;
	case OBJTYPE_FLEET:
		evas_object_image_file_set(icon,
			"edje/images/fleet.png", NULL);
		break;
	default:
		printf("Unknown object!!\n");
		return NULL;
		break;
	}
	//evas_object_resize(icon,64,64);
	evas_object_image_fill_set(icon,0,0,64,64);
	evas_object_pass_events_set(icon,1);

	edje_object_part_swallow(eo, "object", icon);

	edje_object_part_text_set(eo, "Name", obj->name);

	evas_object_event_callback_add(eo, EVAS_CALLBACK_FREE,
				(void*)gui_icon_del_cb, eo);

	if (active){
		int *oid;
		oid = malloc(sizeof(int));
		if (!oid) return eo;
		*oid = obj->oid;
		evas_object_event_callback_add(eo,
						EVAS_CALLBACK_MOUSE_DOWN,
						reference_object_show,
						oid);
		evas_object_data_set(eo, KEY_TPE_GUI, gui);
	}

	return eo;
}

static void
gui_icon_del_cb(Evas_Object *icon){
	Evas_Object *o;
	int *data;
	
	data = evas_object_event_callback_del(icon, EVAS_CALLBACK_MOUSE_DOWN, 
			reference_object_show);
	if (data) free(data);

	o = edje_object_part_swallow_get(icon,"object");
	if (o){
		edje_object_part_unswallow(icon, o);
		evas_object_del(o);
	}
}


static void
reference_object_show(void *idv, Evas *e, Evas_Object *eo, void *event){
	struct gui_obj *guiobj;
	struct gui *gui;
	struct object *obj;
	uint32_t oid;
	Evas_Object *o;

	gui = evas_object_data_get(eo, KEY_TPE_GUI);

	oid = *(uint32_t *)idv;

	obj = tpe_obj_obj_get_by_id(gui->tpe, oid);
	if (obj == NULL){
		/* Object gone?? */
		return;
	}

	guiobj = obj->gui;
	if (guiobj && guiobj->info){
		/* FXME: Warp pointer to window? */
		evas_object_raise(guiobj->info);
	} else {

		o = tpe_ewl_planet_add(gui, obj);
		evas_object_show(o);

		if (guiobj == NULL){
			guiobj = obj->gui = calloc(1,
					sizeof(struct gui_obj));
		}
		guiobj->info = o;
	}
	/* FIXME: This is a bit seedy */
	ecore_job_add((void*)evas_object_raise, guiobj->info);

	gui_window_focus(gui, guiobj->info);

}

/**
 * Gets the gui object structure for a an object.  
 *
 * The gui object contains information about the GUI side of the structure.
 * Note that if the reference to the gui object is to be held, the object
 * delete event must be registered.
 *
 * @param gui Gui system. 
 * @param obj Object to get gui data for.
 * @return Gui data for object, or NULL if not displayed.
 */
struct gui_obj *
gui_object_data_get(struct gui *gui, struct object *obj){
	struct gui_obj *go;
	assert(obj);

	if (obj->gui) return obj->gui;

	go = calloc(1,sizeof(struct gui_obj));
	go->object = obj;
	go->gui = gui;
	obj->gui = go;

	return go;
}


/**
 * Callback handler for a new resoruce being added to the system.
 *
 * Currently only cares about the Home Planet resource
 */
static int 
gui_resource_new(void *data, int eventid, void *event){
	//struct tpe_gui *gui = data;
	struct resourcedescription *rd = event;

	assert(data); assert(event);
	assert(rd->name);

	if (rd->name == NULL) return 1;

	if (strcmp(rd->name,"Home Planet") != 0){
		/* Don't care */
		return 1;
	}

	/* FIXME: Search planet list, and find any with correct resource */

	return 1;
}


/** -------------------------------- 
 * Screen shot code
 */

/**
 * X11 Specific Screengrab 
 */
static int
gui_screengrab(struct gui *gui){
	static int count = 0;
	int w,h,x,y;
	char buf[BUFSIZ];
	//Imlib_Image im;
	const char *path;
	void *ecore_x_display_get(void);

	path = getenv("TPE_SCREENSHOT_DIR");
	if (path == NULL)	
		path = "./";
	snprintf(buf,BUFSIZ,"%s/Screenshot-%03d.png", path, count);
	count ++;

	ecore_evas_geometry_get(gui->ee,&x,&y,&w,&h);
/*
	imlib_context_set_display(ecore_x_display_get());
	imlib_context_set_visual(
			DefaultVisual(ecore_x_display_get(), 
					DefaultScreen(ecore_x_display_get())));
	imlib_context_set_colormap(
			DefaultColormap(ecore_x_display_get(), 
					DefaultScreen(ecore_x_display_get())));
	imlib_context_set_drawable(DefaultRootWindow(ecore_x_display_get()));
	im = imlib_create_image_from_drawable(0, x,y, w, h, 1);
	imlib_context_set_image(im);
	imlib_image_set_format("argb");
	imlib_image_set_format("png");
	imlib_save_image(buf);
	imlib_free_image_and_decache();
*/
	return 0;
}


static int
gui_toggle_names(struct gui *gui){
	const char *signal;
	struct object *obj;

	assert(gui != NULL);
	if (gui->textvisible){
		signal = "HideText";
	} else {
		signal = "ShowText";
	}

	ecore_list_first_goto(gui->visible);
	while ((obj = ecore_list_next(gui->visible))){
		assert(obj != NULL);
		assert(obj->gui != NULL);
		edje_object_signal_emit(obj->gui->obj, signal, "app");
	}
	gui->textvisible = !gui->textvisible;
	
	return 0;
}
