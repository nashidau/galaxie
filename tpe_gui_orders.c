#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include <Evas.h>
#include <Ecore_Data.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#include "tpe_obj.h"

#include "tpe_gui.h"
#include "tpe_gui_private.h"
#include "tpe_gui_orders.h"
#include "tpe_orders.h"
#include "gui_window.h"

static Evas_Object *gui_orders_add(struct gui *gui);
static int gui_orders_object_set(struct gui *gui, Evas_Object *, struct object *obj);
static void gui_orders_cleanup(void *guiv, Evas *e, Evas_Object *ow, void *);



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

	assert(gui);
	if (gui == NULL) return NULL;

	w = edje_object_add(gui->e);
	edje_object_file_set(w,"edje/basic.edj", "OrderWindow");
	evas_object_resize(w, 200,300);
	gui_window_add(gui,w);

	evas_object_event_callback_add(w, EVAS_CALLBACK_FREE, 
			gui_orders_cleanup, gui);

	return w;
}


/**
 * Populate an order window with data.
 */
static int
gui_orders_object_set(struct gui *gui, Evas_Object *ow, struct object *obj){
	const char *orderstr;

	assert(gui); assert(obj); assert(ow);

	edje_object_part_text_set(ow, "Name", obj->name);
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
