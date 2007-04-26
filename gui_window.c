
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include <Evas.h>
#include <Ecore_Data.h>
#include <Ecore_Evas.h>
#include <Ecore_Job.h>
#include <Edje.h>

#include "tpe_obj.h"

#include "tpe_gui.h"
#include "tpe_gui_private.h"
#include "tpe_gui_orders.h"
#include "gui_window.h"

enum {
	WINDOW_LAYER = 10
};


static void gui_window_delete(void *guiv, Evas *e, Evas_Object *window, void *dummy);

int
gui_window_add(struct gui *gui, Evas_Object *window){
	int w,h;

	assert(gui);
	assert(window);

	if (gui->windows == NULL) 
		gui->windows = ecore_list_new();


	ecore_list_prepend(gui->windows, window);
	
	evas_object_layer_set(window,WINDOW_LAYER);

	evas_object_show(window);

	evas_object_geometry_get(window,0,0,&w,&h);
	evas_object_move(window, rand() % (gui->screenw - w),
				rand() % (gui->screenh - h));

	edje_object_signal_callback_add(window,
			"mouse,clicked,*", "Close", 
			(void*)evas_object_del, window);
	evas_object_event_callback_add(window, EVAS_CALLBACK_FREE,
			gui_window_delete, gui);

	return 0;
}

int
gui_window_focus(struct gui *gui, Evas_Object *window){
	evas_object_focus_set(window,1);
	ecore_job_add((void(*)(void*))evas_object_raise,window);
	return 0;
}


static void
gui_window_delete(void *guiv, Evas *e, Evas_Object *window, void *dummy){
	struct gui *gui = guiv;
	assert(gui != NULL); assert(window != NULL);
	assert(gui->windows != NULL);

 	if (ecore_list_goto(gui->windows, window) == NULL){
		assert(!"Could not find item\n");
		return;
	}

	ecore_list_remove(gui->windows);
}
