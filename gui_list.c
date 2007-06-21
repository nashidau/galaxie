#include <inttypes.h>

#include <Ecore_Evas.h>
#include <Evas.h>
#include <Ewl.h>

#include "tpe_gui.h"
#include "tpe_gui_private.h"
#include "gui_window.h"

#include "tpe_obj.h"

static unsigned int 
planet_count_get(void *data){
	return 10;
}


static void *
planet_data_fetch(void *data, unsigned int row, unsigned int column){

	return "planet";
}

static Ewl_Widget *
planet_widget_fetch(void *data, unsigned int row, unsigned int col){
	Ewl_Widget *w;


	w = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(w), planet_data_fetch(data,row,col));

	ewl_widget_show(w);
	return w;
}

static void *
planet_header_data_fetch(void *data, unsigned int col){
	if (col == 0) return "Title";
	else return "Image";
}

static Ewl_Widget *
planet_header_widget_fetch(void *data , unsigned int column){
	Ewl_Widget *l;

	l = ewl_label_new();
	if (column == 0)
		ewl_label_text_set(EWL_LABEL(l), "Who");
	else if (column == 1)
		ewl_label_text_set(EWL_LABEL(l), "Number");
	else
		ewl_label_text_set(EWL_LABEL(l), "Email");
	ewl_widget_show(l);

	return l;
}

void gui_list_planet_add(struct gui *gui){
	Ewl_Widget *win,*wg,*vbox,*tree;
	Ewl_Model *model;
	Ewl_View *view;
	Ecore_List *planets;

	planets = tpe_obj_obj_list_by_type(gui->tpe, OBJTYPE_PLANET);

	win = gui_window_ewl_add(gui);

	vbox = ewl_vbox_new();
	ewl_container_child_append(EWL_CONTAINER(win), vbox);
	ewl_widget_show(vbox);

	wg = ewl_scrollpane_new();
	ewl_container_child_append(EWL_CONTAINER(vbox), wg);
	ewl_object_fill_policy_set(EWL_OBJECT(vbox), EWL_FLAG_FILL_ALL);
	ewl_widget_show(wg);
	
	model = ewl_model_new();
        ewl_model_data_fetch_set(model, planet_data_fetch);
	ewl_model_data_header_fetch_set(model, planet_header_data_fetch);
        ewl_model_data_count_set(model, planet_count_get);

	view = ewl_view_new();
	ewl_view_widget_fetch_set(view, planet_widget_fetch);
	ewl_view_header_fetch_set(view, planet_header_widget_fetch);

        tree = ewl_tree2_new();
        ewl_container_child_append(EWL_CONTAINER(wg), tree);
        ewl_object_fill_policy_set(EWL_OBJECT(tree), EWL_FLAG_FILL_ALL);
	ewl_mvc_model_set(EWL_MVC(tree), model);
	ewl_mvc_view_set(EWL_MVC(tree), view);
	ewl_tree2_column_count_set(EWL_TREE2(tree), 3);
	ewl_widget_show(tree);

	ewl_widget_show(win);
}



