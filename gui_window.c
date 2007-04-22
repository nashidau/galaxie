
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include <Evas.h>
#include <Ecore_Data.h>
#include <Ecore_Evas.h>
#include <Ecore_Job.h>

#include "tpe_obj.h"

#include "tpe_gui.h"
#include "tpe_gui_private.h"
#include "tpe_gui_orders.h"
#include "gui_window.h"


int
gui_window_add(struct gui *gui, Evas_Object *window){
	assert(gui);
	assert(window);

	if (gui->windows == NULL) 
		gui->windows = ecore_list_new();
	
	ecore_list_prepend(gui->windows, window);
	return 0;
}

int
gui_window_focus(struct gui *gui, Evas_Object *window){
	evas_object_focus_set(window,1);
	ecore_job_add((void(*)(void*))evas_object_raise,window);
	return 0;
}
