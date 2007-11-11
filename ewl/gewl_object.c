#include <assert.h>
#include <inttypes.h>
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

void tpe_ewl_planet_set(struct ewl_planet_data *p, struct object *planet);


Evas_Object *
tpe_ewl_planet_add(struct gui *gui, struct object *planet){
	struct ewl_planet_data *p;
	Ewl_Widget *window;
	Ewl_Widget *box;
	
	p = calloc(1,sizeof(struct ewl_planet_data));


	/* FIXME: Move this to a more useful place */
	p->window = window = gui_window_ewl_add(gui);

	box = ewl_hbox_new();
	p->box = box; /* XXX */
	ewl_container_child_append(EWL_CONTAINER(window), box);
	ewl_widget_show(box);

	p->name = ewl_label_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->name);
	ewl_widget_show(p->name);

	p->icon = ewl_icon_simple_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->icon);
	ewl_widget_show(p->icon);


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
		/* We have resources */
		nres = planet->planet->nresources;
		res = planet->planet->resources;
		printf("%d resources\n",nres);
				
		for (i = 0; i <  nres ; i ++){
			rdes = tpe_resources_resourcedescription_get(
					planet->tpe,
					res[i].rid);
			printf("%s: %d %d %d\n",rdes->name,res[i].surface, 
					res[i].minable, res[i].inaccessable);
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
			ewl_widget_show(icon);
		}
	}

}


