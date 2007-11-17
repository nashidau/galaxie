#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <Ecore_Data.h>
#include <Ecore_Evas.h>
#include <Ewl.h>

struct gui;
#include "../tpe_obj.h"
#include "../gui_window.h"
#include "../tpe_orders.h"
#include "../tpe_resources.h"
#include "gewl_object.h"

#define EWL_DATA_MAGIC	"getkdata"
struct ewl_planet_data {
	struct gui *gui;
	//Evas_Object *window;	
	Ewl_Widget *window;

	/* Various pieces */ 
	Ewl_Widget *name;
	Ewl_Widget *icon;
	Ewl_Widget *children;
	Ewl_Widget *resources;
	Ewl_Widget *orders;
	Ewl_Widget *edit;

	struct object *planet;
};

struct ewl_order_data {
	Ewl_Widget *window;

	Ewl_Widget *box;
	Ewl_Widget *pane1;
	Ewl_Widget *curorders;
	Ewl_Widget *posorders;
};

static const char *resheaders[] = {
	"",	/* Icon */
	"Resource",
	"On Surface",
	"Accessable",
	"Inaccessable"
};

static const char *orderheaders[] = {
	"Order",
	"Args"
};

void tpe_ewl_planet_set(struct ewl_planet_data *p, struct object *planet);
char * alloc_printf(const char *format, ...);
static void icon_select(Ewl_Widget *icon, void *ev_data, void *planetv);
static void tpe_ewl_edit_orders(Ewl_Widget *icon, void *ev_data, void *planetv);
static void tpe_ewl_planet_clear(struct ewl_planet_data *p, int everything);
static void tpe_ewl_resource_append(struct tpe *tpe, Ewl_Widget *tree, 
		struct planet_resource *res);
static void tpe_ewl_tree_clear(Ewl_Widget *tree);


Evas_Object *
tpe_ewl_planet_add(struct gui *gui, struct object *planet){
	struct ewl_planet_data *p;
	Ewl_Widget *window;
	Ewl_Widget *box;
	
	p = calloc(1,sizeof(struct ewl_planet_data));
	p->gui = gui;
	


	/* FIXME: Move this to a more useful place */
	p->window = window = gui_window_ewl_add(gui);

	box = ewl_vbox_new();
	ewl_container_child_append(EWL_CONTAINER(window), box);
	ewl_widget_show(box);

	p->name = ewl_label_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->name);
	ewl_widget_show(p->name);

	p->icon = ewl_icon_simple_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->icon);
	ewl_widget_show(p->icon);

	p->resources = ewl_tree_new(5); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(box), p->resources);
	ewl_tree_headers_set(EWL_TREE(p->resources),(char**)resheaders);
	ewl_object_fill_policy_set(EWL_OBJECT(p->resources),EWL_FLAG_FILL_FILL);
	ewl_widget_show(p->resources);

	p->children = ewl_grid_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->children);
	ewl_object_fill_policy_set(EWL_OBJECT(p->children), EWL_FLAG_FILL_FILL);
	ewl_grid_dimensions_set(EWL_GRID(p->children), 3, 3);
	ewl_grid_column_relative_w_set(EWL_GRID(p->children), 0, 0.25);
	ewl_grid_homogeneous_set(EWL_GRID(p->children), 1);
	ewl_grid_row_fixed_h_set(EWL_GRID(p->children), 3, 50);
	ewl_grid_row_preferred_h_use(EWL_GRID(p->children), 2);
	ewl_widget_show(p->children);

	p->orders = ewl_tree_new(2); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(box), p->orders);
	ewl_tree_headers_set(EWL_TREE(p->orders), (char**)orderheaders);
	ewl_object_fill_policy_set(EWL_OBJECT(p->orders), EWL_FLAG_FILL_FILL);
	ewl_widget_show(p->orders);

	/* TODO: Align right */
	p->edit = ewl_button_new();
	ewl_button_label_set(EWL_BUTTON(p->edit), "Edit Orders");
	ewl_container_child_append(EWL_CONTAINER(box), p->edit);
	ewl_object_fill_policy_set(EWL_OBJECT(p->edit), 
			EWL_FLAG_ALIGN_RIGHT | EWL_FLAG_FILL_NONE);
	ewl_callback_append(p->edit, EWL_CALLBACK_CLICKED, 
			tpe_ewl_edit_orders, planet);
	ewl_widget_show(p->edit);

	tpe_ewl_planet_set(p, planet);

	ewl_widget_show(box);
	return NULL;	
}

void
tpe_ewl_planet_set(struct ewl_planet_data *p, struct object *planet){
	const char *file = NULL;
	struct planet_resource *res;
	int nres,i;

	assert(p);
	assert(planet);

	assert(p->window);
	assert(p->name); assert(p->icon);

	tpe_ewl_planet_clear(p, 0);
	
	ewl_label_text_set(EWL_LABEL(p->name), planet->name);

	/* Hack */
	if (planet->type == OBJTYPE_FLEET) file = "edje/images/fleet.png";
	else if(planet->type == OBJTYPE_GALAXY) file="edje/images/galaxy64.png";
	else if(planet->type==OBJTYPE_UNIVERSE)file="edje/images/universe.png";
	else if(planet->type==OBJTYPE_PLANET)file="edje/images/planet.png";
	else if(planet->type==OBJTYPE_SYSTEM)file="edje/images/star.png";
	ewl_icon_image_set(EWL_ICON(p->icon), file, NULL);

	/* Resources */
	/* FIXME: This is TP03 only really */
	/* TP04 should be more generic */
	if (planet->planet){
		/* We have resources */
		nres = planet->planet->nresources;
		res = planet->planet->resources;
				
		for (i = 0; i <  nres ; i ++){
			tpe_ewl_resource_append(planet->tpe,p->resources,res+i);
		}
	}

	if (planet->children){
		/* Build our child list */
		Ewl_Widget *icon;
		struct object **kids;
		int id;
		kids = calloc(planet->nchildren, sizeof(struct tpe_obj *));
		for (i = 0 ; i < planet->nchildren ; i ++){
			id = planet->children[i];
			/* FIXME: Sort here */
			kids[i] = tpe_obj_obj_get_by_id(planet->tpe, id);
		
		}
		/* FIXME: This should be a box or something */
		for (i = 0 ; i < planet->nchildren ; i ++){
			icon = ewl_icon_simple_new();
			if (kids[i]->type == OBJTYPE_PLANET)
				ewl_icon_image_set(EWL_ICON(icon),
						"edje/images/planet.png",NULL);
			else 
				ewl_icon_image_set(EWL_ICON(icon),
						"edje/images/fleet.png",NULL);
			ewl_icon_label_set(EWL_ICON(icon), kids[i]->name);

			ewl_container_child_append(EWL_CONTAINER(p->children),
					icon);
			ewl_grid_child_position_set(EWL_GRID(p->children), icon,
					i % 3, i % 3 + 1, i / 3, i / 3 + 1);
			ewl_callback_append(icon, EWL_CALLBACK_CLICKED, 
					icon_select, kids[i]);
			ewl_widget_show(icon);
			ewl_widget_data_set(icon,"Window",p);
		}
	}

	/* FIXME: Insert orders */

	/* Can we edit orders */
	/* FIXME: Instead of setting window all the time,
	 * 	I should be able to get the toplevel window, and attach data
	 * 	to that one */
	ewl_widget_data_set(p->edit,"Window",p);
	if (planet->nordertypes)
		ewl_widget_enable(p->edit);
	else 
		ewl_widget_disable(p->edit);
	
	p->planet = planet;
}

/**
 * Clear the UI components for a planet structure
 *
 * If everything is set clears all members, including labels.
 * Otherwise just clears lists and elements that are appended to.
 *
 * @param p Planet data
 * @param everything Clear all labels
 */
static void
tpe_ewl_planet_clear(struct ewl_planet_data *p, int everything){
	assert(p);
	if (p == NULL) return;

	if (everything) 
		fprintf(stderr, "%s:%s: Everything flag not implemented\n",
				__FILE__, __FUNCTION__);

	tpe_ewl_tree_clear(p->resources);

	tpe_ewl_tree_clear(p->orders);

	//ewl_grid_init(EWL_GRID(p->children));
	//ewl_grid_dimensions_set(EWL_GRID(p->children), 3,3);
	
}


static void 
tpe_ewl_tree_clear(Ewl_Widget *tree){
	Ewl_Widget *row;

	while ((row = ewl_tree_row_find(EWL_TREE(tree), 0)))
		ewl_tree_row_destroy(EWL_TREE(tree), EWL_ROW(row));

}

static void
tpe_ewl_resource_append(struct tpe *tpe, Ewl_Widget *tree, 
		struct planet_resource *res){
	struct resourcedescription *rdes;
	Ewl_Widget *widgets[5];
	char *labels[4];
	int i;

	rdes = tpe_resources_resourcedescription_get(tpe, res->rid);
	assert(rdes);
	if (rdes == NULL){
		/* FIXME: Handle */
		/* Should insert basic row with formated rdes */
		return;
	}

	/* FIXME: Insert icon */

	labels[0] = rdes->name;
	
	/* FIXME: Implement standard version of these */
	labels[1] = alloc_printf("%d %s",res->surface,
			rdes->unit);
	labels[2] = alloc_printf("%d %s",res->minable,
			rdes->unit);
	labels[3] = alloc_printf("%d %s",res->inaccessable,
			rdes->unit);

	widgets[0] = NULL;
	for (i = 0 ; i < 4 ; i ++){
		widgets[i + 1] = ewl_label_new();
		ewl_label_text_set(EWL_LABEL(widgets[i+1]),labels[i]);
		ewl_widget_show(widgets[i+1]);
	}
		

	ewl_tree_row_add(EWL_TREE(tree), NULL, widgets);
	free(labels[1]); free(labels[2]); free(labels[3]); 

	printf("Res: %s\n",rdes->name); /* XXX: Debug */

}

static void
icon_select(Ewl_Widget *icon, void *ev_data, void *planetv){
	struct ewl_planet_data *p;

	p = ewl_widget_data_get(icon, "Window");
	assert(p);

	tpe_ewl_planet_set(p, planetv);

}


static void
tpe_ewl_edit_orders(Ewl_Widget *button, void *ev_data, void *planetv){
	struct ewl_planet_data *p;
	struct ewl_order_data *od;
	int i;

	p = ewl_widget_data_get(button, "Window");
	assert(p);

	od = calloc(1,sizeof(struct ewl_order_data));

	/* Add the edit orders window */
	od->window = gui_window_ewl_add(p->gui);

	/* Set the data in the edit orders window */
	od->box = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(od->window), od->box);
	ewl_widget_show(od->box);

	od->pane1 = ewl_paned_new();
	ewl_container_child_append(EWL_CONTAINER(od->box), od->pane1);
	ewl_widget_show(od->pane1);

	/* List of current orders */
	od->curorders = ewl_tree_new(2); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(od->pane1), od->curorders);
	ewl_tree_headers_set(EWL_TREE(od->curorders), (char**)orderheaders);
	ewl_object_fill_policy_set(EWL_OBJECT(od->curorders), 
			EWL_FLAG_FILL_FILL);
	ewl_widget_show(od->curorders);

	/* List of possible orders */
	od->posorders = ewl_tree_new(1); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(od->pane1), od->posorders);
	ewl_tree_headers_set(EWL_TREE(od->posorders), (char**)orderheaders);
	ewl_object_fill_policy_set(EWL_OBJECT(od->posorders), 
			EWL_FLAG_FILL_FILL);
	ewl_widget_show(od->posorders);

	ewl_widget_show(od->window);


	/* Seed possible orders on this object */
	printf("Nordertypes: %s %d!\n", p->planet->name, p->planet->nordertypes);
	for (i = 0 ; i < p->planet->nordertypes ; i ++){
		Ewl_Widget *label;
		uint32_t otype;
		const char *oname;
		otype = p->planet->ordertypes[i];
		oname = tpe_order_get_name_by_type(p->planet->tpe, otype);
		if (oname == NULL){
			printf("Warning: Unable to find order for %d\n",otype);
			continue;
		}
		printf("oname: %s\n",oname);
		label = ewl_label_new();
		ewl_label_text_set(EWL_LABEL(label), oname);
		ewl_widget_show(label);

		ewl_tree_row_add(EWL_TREE(od->posorders), NULL, &label);
	}
	


}

/* FIXME: Move to tpe_util */
char *
alloc_printf(const char *fmt, ...){
	va_list ap;
	int count;
	char *buf;

	va_start(ap, fmt);
	count = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	buf = calloc(1,count + 1);
	va_start(ap, fmt);
	vsnprintf(buf, count + 1, fmt, ap);
	va_end(ap);

	return buf;
}
