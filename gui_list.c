#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <Ecore_Evas.h>
#include <Evas.h>
#include <Ewl.h>

#include "tpe_gui.h"
#include "tpe_gui_private.h"
#include "gui_window.h"
#include "tpe_obj.h"

enum listtype {
	LIST_IMAGE,
	LIST_TEXT,
};

struct guilist {
	enum listtype type;
	const char *header;
};



static const struct guilist planetlist[] = {
	//{ LIST_IMAGE, "Image" },
	{ LIST_TEXT, "Planet" },
	{ LIST_TEXT, "Parent" },
	{ LIST_TEXT, "Owner" },
};

struct listsource {
	struct gui *gui;
	struct guilist *format;
	Ecore_List *list;
};	

static unsigned int 
planet_count_get(void *data){
	struct listsource *source = data;

	return ecore_list_nodes(source->list);
}


static void *
planet_data_fetch(void *data, unsigned int row, unsigned int column){
	struct listsource *source = data;
	struct object *o,*p;

	o = ecore_list_goto_index(source->list,row);
	/* FIXME: Check all args */
	if (column == 0)
		return o->name;
	else if (column == 1){
		p = tpe_obj_obj_get_by_id(source->gui->tpe, o->parent);
		if (p)
			return p->name;
		else
			return "Unknown";
	} else if (column == 2){
		return "stuff";
	}
	return "planet";
}

static Ewl_Widget *
planet_widget_fetch(void *data, unsigned int row, unsigned int col){
	Ewl_Widget *w;

	w = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(w), data);

	ewl_widget_show(w);
	return w;
}

static void *
planet_header_data_fetch(void *data, unsigned int col){
	struct listsource *source = data;

	return source->format[col].header;
}

static Ewl_Widget *
planet_header_widget_fetch(void *data, unsigned int column){
	struct listsource *list = data;
	Ewl_Widget *l;
	
	/* FIXME: Should sanity check column */
	l = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(l), data);
	ewl_widget_show(l);

	return l;
}

void gui_list_planet_add(struct gui *gui){
	struct listsource *list;
	Ewl_Widget *win,*wg,*vbox,*tree;
	Ewl_Model *model;
	Ewl_View *view;
	Ecore_List *planets;

	planets = tpe_obj_obj_list_by_type(gui->tpe, OBJTYPE_PLANET);

	list = calloc(1,sizeof(struct listsource));
	list->gui = gui;
	list->list = planets;
	list->format = planetlist;
	printf("List is %p\n",list);

	/* Most of this needs to be put into a subfunction */

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
	ewl_tree2_column_count_set(EWL_TREE2(tree), 3); /* FIXME */
	ewl_mvc_data_set(EWL_MVC(tree), list); /* FIXME */
	ewl_widget_show(tree);

	ewl_widget_show(win);
}



