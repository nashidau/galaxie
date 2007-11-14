
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <Ecore_Data.h>
#include <Ecore_Evas.h>
#include <Ecore_Job.h>
#include <Edje.h>
#include <Evas.h>
#include <Ewl.h>

#include "tpe_obj.h"

#include "tpe_gui.h"
#include "tpe_gui_private.h"
#include "tpe_gui_orders.h"
#include "gui_window.h"

enum {
	WINDOW_LAYER = 10
};


static void gui_window_delete(void *guiv, Evas *e, Evas_Object *window, void *dummy);

Evas_Object *
gui_window_add2(struct gui *gui, const char *title, const char *icon){
	Evas_Object *window;

	assert(gui);
	
	window = edje_object_add(gui->e);
	edje_object_file_set(window,"edje/basic.edj", "Window");
	evas_object_show(window);
	evas_object_resize(window,300,400);
	
	if (title)
		edje_object_part_text_set(window,"Title", title);
	
	if (icon){
		printf(__FILE__ ":FIXME: "
			"setting icon not implemented\n");
	}

	ecore_list_prepend(gui->windows, window);

	edje_object_signal_callback_add(window,
			"mouse,clicked,*", "Close", 
			(void*)evas_object_del, window);
	evas_object_event_callback_add(window, EVAS_CALLBACK_FREE,
			gui_window_delete, gui);

	return window;
}

/** FIXME: This should create the basic window object */
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

	evas_object_geometry_get(window,NULL,NULL,&w,&h);
	evas_object_move(window, rand() % (gui->screenw - w),
				rand() % (gui->screenh - h));

	edje_object_signal_callback_add(window,
			"mouse,clicked,*", "Close", 
			(void*)evas_object_del, window);
	evas_object_event_callback_add(window, EVAS_CALLBACK_FREE,
			gui_window_delete, gui);

	return 0;
}


Ewl_Widget *
gui_window_ewl_add(struct gui *gui){
	Ewl_Widget *emb;
	Evas_Object *eo;
	Evas_Object *window;
	
	window = edje_object_add(gui->e);
	edje_object_file_set(window,"edje/basic.edj", "Window");
	evas_object_show(window);
	evas_object_resize(window,400,400);

	emb = ewl_embed_new();
	ewl_object_fill_policy_set(EWL_OBJECT(emb), EWL_FLAG_FILL_ALL);
	eo = ewl_embed_canvas_set(EWL_EMBED(emb), gui->e, 
			(void *) ecore_evas_software_x11_window_get(gui->ee));
	ewl_embed_focus_set(EWL_EMBED(emb), TRUE);
	ewl_widget_show(emb);

	edje_object_part_swallow(window, "swallow", eo);
	evas_object_data_set(window, "EWL", emb);

	gui_window_add(gui, window);

	return emb;
}

int
gui_window_focus(struct gui *gui, Evas_Object *window){
	evas_object_focus_set(window,1);
	ecore_job_add((void(*)(void*))evas_object_raise,window);
	return 0;
}


static void
gui_window_delete(void *guiv, Evas *e, Evas_Object *window, void *dummy){
	Ewl_Widget *emb;
	struct gui *gui = guiv;
	assert(gui != NULL); assert(window != NULL);
	assert(gui->windows != NULL);

 	if (ecore_list_goto(gui->windows, window) == NULL){
		assert(!"Could not find item\n");
		return;
	}

	/* FIXME: Maybe not the best strategy */
	evas_object_focus_set(gui->main,1);

	emb = evas_object_data_get(window,"EWL");
	if (emb)
		ewl_widget_destroy(emb);

	ecore_list_remove(gui->windows);
}
