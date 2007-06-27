#include <assert.h>
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

static const struct guilist orderlist[] = {
	{ LIST_TEXT, "Order" },
};


struct listsource {
	struct gui *gui;
	const struct guilist *format;
	/* FIXME: Need to make this an array for direct seeks and sorting */
	Ecore_List *list;

	int ndata;
	void **data;
};	

void *orders_data_fetch(void *data, unsigned int row, unsigned int col);
static Ewl_Widget *orders_widget_fetch(void *data, unsigned int, unsigned int col);






static unsigned int 
planet_count_get(void *data){
	struct listsource *source = data;
printf("count get\n");
	if (source->ndata)
		return source->ndata;
	else
		return ecore_list_nodes(source->list);
}


static void *
planet_data_fetch(void *data, unsigned int row, unsigned int column){
	struct listsource *source = data;
	struct object *o,*p;

printf("pdf\n");	
	if (source->list){
		/* FIXME: Ditch this data type !! */
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
	} else {
		printf("returning %s\n",(char *)source->data[row]);
		return source->data[row];
	}
	return "planet";
}

static Ewl_Widget *
planet_widget_fetch(void *data, unsigned int row, unsigned int col){
	Ewl_Widget *w;

printf("pwf\n");	
	w = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(w), data);

	ewl_widget_show(w);
	return w;
}

static void *
planet_header_data_fetch(void *data, unsigned int col){
	struct listsource *source = data;

printf("phdf\n");	
	/* Discarding const ness */
	return (void*)source->format[col].header;
}

static Ewl_Widget *
planet_header_widget_fetch(void *data, unsigned int column){
	Ewl_Widget *l;
printf("phwf\n");	
	/* FIXME: Should sanity check column */
	l = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(l), data);
	ewl_widget_show(l);

	return l;
}

Ewl_Widget *
gui_list_orders_add(struct gui *gui){
	struct listsource *list;
	Ewl_Widget *scroll,*tree;
	Ewl_Model *model;
	Ewl_View *view;

	assert(gui);
printf("orders add\n");
	list = calloc(1,sizeof(struct listsource));
	list->gui = gui;
	list->format = orderlist;
	list->list = NULL;

	/* BIG FIXME */
	list->ndata = 2;
	list->data = malloc(2 * sizeof(char *));
	list->data[0] = "Order 1";
	list->data[1] = "Order 2";

	scroll = ewl_scrollpane_new();
	ewl_widget_show(scroll);

	model = ewl_model_new();
        ewl_model_data_fetch_set(model, planet_data_fetch);
	ewl_model_data_header_fetch_set(model, planet_header_data_fetch);
        ewl_model_data_count_set(model, planet_count_get);

	view = ewl_view_new();
	ewl_view_widget_fetch_set(view, orders_widget_fetch);
	ewl_view_header_fetch_set(view, planet_header_widget_fetch);

        tree = ewl_tree2_new();
        ewl_container_child_append(EWL_CONTAINER(scroll), tree);
        ewl_object_fill_policy_set(EWL_OBJECT(tree), EWL_FLAG_FILL_ALL);
	ewl_mvc_model_set(EWL_MVC(tree), model);
	ewl_mvc_view_set(EWL_MVC(tree), view);
	ewl_tree2_column_count_set(EWL_TREE2(tree), 2); /* FIXME */
	ewl_mvc_data_set(EWL_MVC(tree), list); /* FIXME */
	ewl_widget_show(tree);
printf("returning %p\n");
	return scroll;
}

void
gui_list_orders_set(struct gui *gui, Ewl_Widget *widget, struct object *obj){
	printf("gui_list_orders_set called\n");
}

static 
Ewl_Widget *orders_widget_fetch(void *data, unsigned int row, unsigned int col){
printf("owf\n");
	return data;
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
	/* FIXME: discarding constness */
	list->format = (void*)planetlist;
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



