#include <assert.h>

#include "tpe_gui.h"
#include "tpe_gui_orders.h"


/* Edit the orders on an object */
int 
gui_orders_edit(struct gui *gui, struct object *obj){
	struct gui_obj *go;
	Evas_Object *objectwindow;

	assert(gui);
	assert(obj);
	assert(obj->nordertypes >= 0);

	if (gui == NULL || obj == NULL) return -1;
	if (obj->nordertypes == 0) return -2;

	go = gui_object_data_get(gui,obj);

	if (go->orders == NULL)
		go->orders = gui_orders_add(gui);
	
}

