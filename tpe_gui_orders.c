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
#include "gui_window.h"

static Evas_Object *gui_orders_add(struct gui *gui);
static int gui_orders_object_set(struct gui *gui, Evas_Object *, struct object *obj);




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

	if (go->orders == NULL)
		go->orders = gui_orders_add(gui);
	if (go->orders == NULL) return -1;

	gui_orders_object_set(gui, go->orders, obj);	

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

	gui_window_add(gui,w);

	return w;
}

/**
 * Populate an order window with data.
 */
static int
gui_orders_object_set(struct gui *gui, Evas_Object *o, struct object *obj){

	assert(gui);
	assert(obj);

	return -1;
}


