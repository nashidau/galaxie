#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <Ecore_Data.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Evas.h>
#include <Ewl.h>

#include "tpe_obj.h"

#include "tpe_gui.h"
#include "tpe_gui_private.h"
#include "tpe_gui_orders.h"
#include "tpe_orders.h"
#include "gui_window.h"
#include "gui_list.h"

static Evas_Object *gui_orders_add(struct gui *gui);
static int gui_orders_object_set(struct gui *gui, Evas_Object *, struct object *obj);
static void gui_orders_cleanup(void *guiv, Evas *e, Evas_Object *ow, void *);

/*
 * Format of order window:
 *   List of current orders
 *
 *   Currently selected order type [drop down]
 *   Params for order type
 */

struct orderwindow {
	Ewl_Widget *box;  /* The box to append stuff to */
	Ewl_Widget *list; /* List of selection items */
	Ewl_Widget *sel;  /* Drop down for current order type */
	Ewl_Widget *parambox; /* The box for the current widget */
	struct object *obj; /* The object which this is orders for */
};


/* Edit the orders on an object */
int 
gui_orders_edit(struct gui *gui, struct object *obj){
	struct gui_obj *go;

	assert(gui);
	assert(obj);
	assert(obj->nordertypes >= 0);

	if (gui == NULL || obj == NULL) return -1;
	if (obj->nordertypes == 0) return -2;

	go = gui_object_data_get(gui,obj);

	if (go->orders == NULL){
		go->orders = gui_orders_add(gui);
		if (go->orders == NULL) return -1;
		gui_orders_object_set(gui, go->orders, obj);	
	}

	gui_window_focus(gui, go->orders);

	return 0;
}

/**
 * Adds an order window.
 *
 * It will be initially unpopulated, use gui_ordres_object_set to populaet it.
 * It will be shown.
 *
 * @param gui Gui data structure 
 * @return New orders window, or NULL on error 
 */
static Evas_Object *
gui_orders_add(struct gui *gui){
	Evas_Object *w;
	Ewl_Widget *emb;
	Evas_Object *eo;
	Ewl_Widget *box,*list;
	struct orderwindow *info;

	assert(gui);
	if (gui == NULL) return NULL;

	info = calloc(1,sizeof(struct orderwindow));

	w = gui_window_add2(gui,"Orders",NULL);
	evas_object_event_callback_add(w, EVAS_CALLBACK_FREE, 
			gui_orders_cleanup, gui);
	evas_object_data_set(w,"OrderWindow", info);
	
	/* FIXME: This needs to go somewhere else */
	emb = ewl_embed_new();
	ewl_object_fill_policy_set(EWL_OBJECT(emb), EWL_FLAG_FILL_ALL);
	eo = ewl_embed_canvas_set(EWL_EMBED(emb), gui->e, 
			(void *) ecore_evas_software_x11_window_get(gui->ee));
	ewl_embed_focus_set(EWL_EMBED(emb), TRUE);
	ewl_widget_show(emb);

	edje_object_part_swallow(w, "swallow", eo);

	/* Add vbox for stuff to go into */
	box = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(emb), box);
	info->box = box;
	ewl_widget_show(box);

	/* Add list for items */
	list = gui_list_orders_add(gui);
	ewl_container_child_append(EWL_CONTAINER(box), list);
	ewl_object_fill_policy_set(EWL_OBJECT(list), EWL_FLAG_FILL_ALL);
	ewl_widget_show(list);



	{ int x,y,w,h;
	evas_object_geometry_get(eo,&x,&y,&w,&h);
	printf("%d %d %d %d\n",x,y,w,h);
	}

	return w;
}


/**
 * Populate an order window with data.
 */
static int
gui_orders_object_set(struct gui *gui, Evas_Object *ow, struct object *obj){
	const char *orderstr;

	assert(gui); assert(obj); assert(ow);

	edje_object_part_text_set(ow, "Title", obj->name);
	orderstr = tpe_orders_str_get(gui->tpe, obj);
	edje_object_part_text_set(ow, "Orders", orderstr);

	evas_object_data_set(ow, "Object", obj);

	return 0;
}



static void
gui_orders_cleanup(void *guiv, Evas *e, Evas_Object *ow, void *dummy){
	struct gui_obj *go;
	struct gui *gui = guiv;
	struct object *obj;

	assert(gui); assert(ow); assert(e);
	if (gui == NULL || ow == NULL) return;

	obj = evas_object_data_get(ow, "Object");
	assert(obj);
	if (obj == NULL) return;

	go = obj->gui;
	assert(go != NULL);
	assert(go->orders != NULL);

	go->orders = NULL;
	
	/* Nothing else to free now */

	return;
}
