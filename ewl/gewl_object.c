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
#include "../tpe_resources.h"
#include "gewl_object.h"

#define EWL_DATA_MAGIC	"getkdata"
struct ewl_planet_data {
	//Evas_Object *window;	
	Ewl_Widget *window;

	/* FIXME: Hack for the planet_set call */
	Ewl_Widget *box;
	
	/* Various pieces */ 
	Ewl_Widget *name;
	Ewl_Widget *icon;
	Ewl_Widget *children;
	Ewl_Widget *resources;
};

static const char *resheaders[] = {
	"Resource",
	"On Surface",
	"Accessable",
	"Inaccessable"
};

void tpe_ewl_planet_set(struct ewl_planet_data *p, struct object *planet);
char * alloc_printf(const char *format, ...);
static void icon_select(Ewl_Widget *icon, void *ev_data, void *planetv);


Evas_Object *
tpe_ewl_planet_add(struct gui *gui, struct object *planet){
	struct ewl_planet_data *p;
	Ewl_Widget *window;
	Ewl_Widget *box;
	
	p = calloc(1,sizeof(struct ewl_planet_data));


	/* FIXME: Move this to a more useful place */
	p->window = window = gui_window_ewl_add(gui);

	box = ewl_vbox_new();
	p->box = box; /* XXX */
	ewl_container_child_append(EWL_CONTAINER(window), box);
	ewl_widget_show(box);

	p->name = ewl_label_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->name);
	ewl_widget_show(p->name);

	p->icon = ewl_icon_simple_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->icon);
	ewl_widget_show(p->icon);

	p->resources = ewl_tree_new(4); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(box), p->resources);
	ewl_object_fill_policy_set(EWL_OBJECT(p->resources),EWL_FLAG_FILL_FILL);
	ewl_tree_headers_set(EWL_TREE(p->resources),(char**)resheaders);
	ewl_widget_show(p->resources);

	p->children = ewl_grid_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->children);
	ewl_object_fill_policy_set(EWL_OBJECT(p->children), EWL_FLAG_FILL_FILL);
	ewl_grid_dimensions_set(EWL_GRID(p->children), 2, 2);
	//ewl_grid_column_relative_w_set(EWL_GRID(p->children), 0, 0.25);
	ewl_grid_homogeneous_set(EWL_GRID(p->children), 1);
	//ewl_grid_row_fixed_h_set(EWL_GRID(p->children), 3, 50);
	//ewl_grid_row_preferred_h_use(EWL_GRID(p->children), 2);
	ewl_widget_show(p->children);


	tpe_ewl_planet_set(p, planet);

	ewl_widget_show(box);
	return NULL;	
}

void
tpe_ewl_planet_set(struct ewl_planet_data *p, struct object *planet){
	const char *file = NULL;
	struct planet_resource *res;
	struct resourcedescription *rdes;
	int nres,i;

	assert(p);
	assert(planet);

	assert(p->window);
	assert(p->name); assert(p->icon);

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
		char *resdata[4];
		Ewl_Widget *widgets[4];
		/* We have resources */
		nres = planet->planet->nresources;
		res = planet->planet->resources;
				
		for (i = 0; i <  nres ; i ++){
			rdes = tpe_resources_resourcedescription_get(
					planet->tpe,
					res[i].rid);
			resdata[0] = rdes->name;
			/* FIXME: Implement standard version of these */
			resdata[1] = alloc_printf("%d %s",res[i].surface,
					rdes->unit);
			resdata[2] = alloc_printf("%d %s",res[i].minable,
					rdes->unit);
			resdata[3] = alloc_printf("%d %s",res[i].inaccessable,
					rdes->unit);

			widgets[0] = ewl_label_new();
			ewl_label_text_set(widgets[0],resdata[0]);
			ewl_widget_show(widgets[0]);
			widgets[1] = ewl_label_new();
			ewl_label_text_set(widgets[1],resdata[1]);
			ewl_widget_show(widgets[1]);
			widgets[2] = ewl_label_new();
			ewl_label_text_set(widgets[2],resdata[2]);
			ewl_widget_show(widgets[2]);
			widgets[3] = ewl_label_new();
			ewl_label_text_set(widgets[3],resdata[3]);
			ewl_widget_show(widgets[3]);

			ewl_tree_row_add(EWL_TREE(p->resources), NULL, widgets);
			free(resdata[1]); free(resdata[2]); free(resdata[3]); 

			printf("Res: %s\n",rdes->name);
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
				ewl_icon_image_set(EWL_ICON(icon),"edje/images/planet.png",NULL);
			else 
				ewl_icon_image_set(EWL_ICON(icon),"edje/images/fleet.png",NULL);

			ewl_container_child_append(EWL_CONTAINER(p->box), icon);
			ewl_callback_append(icon, EWL_CALLBACK_CLICKED, 
					icon_select, kids[i]);
			ewl_widget_show(icon);
			ewl_widget_data_set(icon,"Window",p);
		}
	}

}

static void
icon_select(Ewl_Widget *icon, void *ev_data, void *planetv){
	struct ewl_planet_data *p;

	p = ewl_widget_data_get(icon, "Window");
	assert(p);

	tpe_ewl_planet_set(p, planetv);

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
