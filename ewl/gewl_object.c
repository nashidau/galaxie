#include <assert.h>
#include <inttypes.h>

#include <Ecore_Data.h>
#include <Ecore_Evas.h>
#include <Ewl.h>

#include "../tpe_obj.h"
#include "../gui_window.h"

struct gui;

#define ETK_DATA_MAGIC	"getkdata"
struct etk_data {
	
};

static void
window_pos_get(void *data, int *x, int *y){
	ecore_evas_geometry_get(data, x,y, NULL,NULL);
}

Evas_Object *
tpe_ewl_planet_add(struct gui *gui, struct object *planet){
	Ewl_Widget *window;
	Ewl_Widget *w;
	Ewl_Widget *box;
	Evas *e;

	/* FIXME: Move this to a more useful place */
	window = gui_window_ewl_add(gui);

	box = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(window), box);
	ewl_widget_show(box);
	
	w = ewl_label_new();
	ewl_label_text_set(w,planet->name);
	ewl_container_child_append(EWL_CONTAINER(box), w);
	ewl_widget_show(w);



	/* FIXME */	
	return NULL;	
}




