#include <assert.h>
#include <inttypes.h>
#include <math.h>
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
	Ewl_Widget *addorders;
	Ewl_Widget *argbox;
	Ewl_Widget *posorders;

	struct object *planet;

	struct order *order;
	struct order_desc *desc;

	void **widgetdata;
};

struct gewl_arg_list {
	Ewl_Widget *box;
	Ewl_Model *model;
	Ewl_View *view;

	struct order_arg_list *list;

	struct gewl_arg_list_selections *selections;
};
struct gewl_arg_list_selections {
	struct gewl_arg_list_selections *next;
	Ewl_Widget *combo;
	Ewl_Widget *spinner;
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
static void order_probe_cb(void *odv, struct object *obj, struct order_desc *desc, struct order *order);


static void order_type_selected(Ewl_Widget *row, void *edata, void *pdata);
static void gewl_arg_coords(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);
static void gewl_arg_turns(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);
static void gewl_arg_object(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);
static void gewl_arg_player(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);
static void gewl_arg_relcoords(struct ewl_order_data *od, struct order_arg *,
		union order_arg_data *);
static void gewl_arg_range(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);
static void gewl_arg_list(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);
static void gewl_arg_string(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);
static void gewl_arg_ref(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);
static void gewl_arg_reflist(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *);

static int coords_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int turn_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int object_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int player_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int relc_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int range_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int list_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int string_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int ref_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);
static int refl_save(struct ewl_order_data *id, struct order_arg *,
		union order_arg_data *);



static Ewl_Widget * gewl_arg_list_header_fetch(void *data, unsigned int col);
static void *gewl_arg_list_data_fetch(void *data, unsigned int row, 
		unsigned int col);
static unsigned int gewl_arg_list_count_get(void *data);
static void gewl_arg_list_value_changed(Ewl_Widget *widget, void *event,void*);
static void gewl_arg_list_add_row_cb(Ewl_Widget *widget, void *data, void *gal);
static Ewl_Widget *gewl_arg_list_row_add(struct gewl_arg_list *gal);

static void gewl_order_submit(Ewl_Widget *widget, void *data, void *gal);

static Ewl_Widget *gewl_box_with_label(const char *str, Ewl_Widget *parent);

struct arg_handler {
	void (*create)(struct ewl_order_data *od,
			struct order_arg *, union order_arg_data *);
	int (*save)(struct  ewl_order_data *od, struct order_arg *,
				union order_arg_data *);
};

static const struct arg_handler arg_handlers[] = {
	{ /* 0: Coords */ gewl_arg_coords,	coords_save	},
	{ /* 1: Turns */  gewl_arg_turns,	turn_save	},
	{ /* 2: Object */ gewl_arg_object,	object_save	},
	{ /* 3: Player */ gewl_arg_player,	player_save	},
	{ /* 4: RelCoords */ gewl_arg_relcoords,relc_save	},
	{ /* 5: Range */  gewl_arg_range,	range_save	},
	{ /* 6: List */   gewl_arg_list,	list_save	},
	{ /* 7: String */ gewl_arg_string,  	string_save	},
	{ /* 8: Ref */	  gewl_arg_ref,		ref_save	},
	{ /* 9: Reflist */gewl_arg_reflist,	refl_save	},
};
#define N_ARGTYPES (sizeof(arg_handlers)/sizeof(arg_handlers[0]))

Evas_Object *
tpe_ewl_planet_add(struct gui *gui, struct object *planet){
	struct ewl_planet_data *p;
	Ewl_Widget *window;
	Ewl_Widget *box;
	
	p = calloc(1,sizeof(struct ewl_planet_data));
	p->gui = gui;
	


	/* FIXME: Move this to a more useful place */
	p->window = window = gui_window_ewl_add(gui, planet->name, NULL);

	box = ewl_vbox_new();
	ewl_container_child_append(EWL_CONTAINER(window), box);
	ewl_widget_show(box);

	p->name = ewl_label_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->name);
	ewl_widget_show(p->name);

	p->icon = ewl_icon_simple_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->icon);
	ewl_widget_show(p->icon);

	p->resources = ewl_tree_new(); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(box), p->resources);
#ifdef NEWTREE
	ewl_tree_headers_set(EWL_TREE(p->resources),(char**)resheaders);
	ewl_object_fill_policy_set(EWL_OBJECT(p->resources),EWL_FLAG_FILL_FILL);
	ewl_widget_show(p->resources);
#endif

	p->children = ewl_grid_new();
	ewl_container_child_append(EWL_CONTAINER(box), p->children);
	ewl_object_fill_policy_set(EWL_OBJECT(p->children), EWL_FLAG_FILL_FILL);
	ewl_grid_dimensions_set(EWL_GRID(p->children), 3, 3);
	ewl_grid_column_relative_w_set(EWL_GRID(p->children), 0, 0.25);
	ewl_grid_homogeneous_set(EWL_GRID(p->children), 1);
	ewl_grid_row_fixed_h_set(EWL_GRID(p->children), 3, 50);
	ewl_grid_row_preferred_h_use(EWL_GRID(p->children), 2);
	ewl_widget_show(p->children);
#if 0
	p->orders = ewl_tree_new(2); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(box), p->orders);
	ewl_tree_headers_set(EWL_TREE(p->orders), (char**)orderheaders);
	ewl_object_fill_policy_set(EWL_OBJECT(p->orders), EWL_FLAG_FILL_FILL);
	ewl_widget_show(p->orders);
#endif
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
	if (planet->planet && planet->planet->nresources){
		/* We have resources */
		nres = planet->planet->nresources;
		res = planet->planet->resources;
			
		//ewl_widget_show(p->resources);
		for (i = 0; i <  nres ; i ++){
			tpe_ewl_resource_append(planet->tpe,p->resources,res+i);
		}
	} else {
		//ewl_widget_hide(p->resources);
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
#ifdef NEWTREE
	tpe_ewl_tree_clear(p->resources);

	tpe_ewl_tree_clear(p->orders);
#endif
	//ewl_grid_init(EWL_GRID(p->children));
	//ewl_grid_dimensions_set(EWL_GRID(p->children), 3,3);
	
}


static void 
tpe_ewl_tree_clear(Ewl_Widget *tree){
#ifdef NEWTREE
	Ewl_Widget *row;
	while ((row = ewl_tree_row_find(EWL_TREE(tree), 0)))
		ewl_tree_row_destroy(EWL_TREE(tree), EWL_ROW(row));
#endif
}

static void
tpe_ewl_resource_append(struct tpe *tpe, Ewl_Widget *tree, 
		struct planet_resource *res){
#ifdef NEWTREE
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
#endif
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
	od->planet = p->planet;

	/* Add the edit orders window */
	od->window = gui_window_ewl_add(p->gui, p->planet->name, NULL);
/* FIXME: This shoudl be in the set call */
	ewl_widget_data_set(od->window, "Object", od);

	/* Set the data in the edit orders window */
	od->box = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(od->window), od->box);
	ewl_widget_show(od->box);

	od->pane1 = ewl_paned_new();
	ewl_container_child_append(EWL_CONTAINER(od->box), od->pane1);
	ewl_widget_show(od->pane1);

	/* List of current orders */
#ifdef NEWTREE
	od->curorders = ewl_tree_new(); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(od->pane1), od->curorders);
	ewl_tree_headers_set(EWL_TREE(od->curorders), (char**)orderheaders);
	ewl_object_fill_policy_set(EWL_OBJECT(od->curorders), 
			EWL_FLAG_FILL_FILL);
	ewl_widget_show(od->curorders);
#endif
	od->addorders = ewl_vbox_new();
	ewl_container_child_append(EWL_CONTAINER(od->pane1), od->addorders);
	ewl_object_fill_policy_set(EWL_OBJECT(od->addorders), 
			EWL_FLAG_FILL_FILL);
	ewl_widget_show(od->addorders);

	/* List of possible orders */
#ifdef NEWTREE
	od->posorders = ewl_tree_new(1); /* XXX */
	ewl_container_child_append(EWL_CONTAINER(od->addorders), od->posorders);
	ewl_tree_headers_set(EWL_TREE(od->posorders), (char**)orderheaders);
	ewl_object_fill_policy_set(EWL_OBJECT(od->posorders), 
			EWL_FLAG_FILL_FILL);
	ewl_tree_mode_set(EWL_TREE(od->posorders), EWL_SELECTION_MODE_SINGLE);
	ewl_widget_show(od->posorders);
#endif
	ewl_widget_show(od->window);

	/* Arguments for orders */
	od->argbox = ewl_vbox_new();
	ewl_container_child_append(EWL_CONTAINER(od->addorders), od->argbox);
	ewl_object_fill_policy_set(EWL_OBJECT(od->argbox), 
			EWL_FLAG_FILL_FILL);
	ewl_widget_show(od->argbox);


	/* Seed possible orders on this object */
	for (i = 0 ; i < p->planet->nordertypes ; i ++){
		Ewl_Widget *label;
		Ewl_Widget *row;
		uint32_t otype;
		const char *oname;
		otype = p->planet->ordertypes[i];
		oname = tpe_order_get_name_by_type(p->planet->tpe, otype);
		if (oname == NULL){
			printf("Warning: Unable to find order for %d\n",otype);
			continue;
		}
		label = ewl_label_new();
		ewl_label_text_set(EWL_LABEL(label), oname);
		ewl_widget_show(label);
#ifdef NEWTREE
		row = ewl_tree_row_add(EWL_TREE(od->posorders), NULL, &label);
		ewl_callback_append(row, EWL_CALLBACK_CLICKED, 
				order_type_selected, (void*)otype);
#endif
	}

}

static void
order_type_selected(Ewl_Widget *row, void *edata, void *otypev){
	uint32_t otype;
	Ewl_Widget *parent,*next;
	struct object *planet;
	struct ewl_order_data *od;

	otype = (uint32_t)otypev;

	parent = row;
	while ((next = ewl_widget_parent_get(parent)))
		parent = next;
	assert(parent);
	od = ewl_widget_data_get(parent, "Object");
	assert(od);
	planet = od->planet;

	ewl_container_reset(EWL_CONTAINER(od->argbox));
	gewl_box_with_label("Getting Args...", od->argbox);

	tpe_orders_object_probe(planet->tpe, planet, otype, order_probe_cb, od);
}

static void
order_probe_cb(void *odv, struct object *obj, struct order_desc *desc,
		struct order *order){
	struct ewl_order_data *od = odv;
	Ewl_Widget *button;
	int i;

	assert(od);

	ewl_container_reset(EWL_CONTAINER(od->argbox));

	printf("Order is a %s\n", desc->name);
	od->desc = desc;
	od->order = order;

	for (i = 0 ; i < desc->nargs ; i ++){
		printf("Arg is a %s: %d\n",desc->args[i].name,
				desc->args[i].arg_type);
		assert(desc->args[i].arg_type < N_ARGTYPES);
		if (desc->args[i].arg_type >= N_ARGTYPES){
			printf("\tUnhandled arg type: %d\n",
					desc->args[i].arg_type);
			continue;
		}
		arg_handlers[desc->args[i].arg_type].create(od,desc->args + i,
				order->args[i]);
	}

	/* Now add the submit button! */
	button = ewl_button_new();
	ewl_button_label_set(EWL_BUTTON(button), "Submit");
	ewl_container_child_append(EWL_CONTAINER(od->argbox), button);
	ewl_callback_append(button, EWL_CALLBACK_CLICKED, 
			gewl_order_submit, od);
	ewl_widget_show(button);
	
}


static void 
gewl_order_submit(Ewl_Widget *widget, void *data, void *odv){
	struct ewl_order_data *od = odv;
	struct order_desc *desc;
	struct order *order;
	int i;


	desc = od->desc;
	order = od->order;

	for (i = 0 ; i < desc->nargs ; i ++){
		if (desc->args[i].arg_type >= N_ARGTYPES){
			printf("\tUnhandled arg type: %d\n",
					desc->args[i].arg_type);
			continue;
		}
		arg_handlers[desc->args[i].arg_type].save(od,desc->args + i,
				order->args[i]);
	}

	/* FIXME: Need to update UI, and display a message until it is
	 * submittied */

	tpe_orders_order_update(od->planet->tpe, order);
}


static void
gewl_arg_string(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	Ewl_Widget *box;
	Ewl_Widget *entry;
	struct order_arg_string *str;

	assert(od);
	assert(arg);
	assert(orderinfo);

	str = &(orderinfo->string);

	box = gewl_box_with_label(arg->name, od->argbox);

	entry = ewl_entry_new();
	if (str->maxlen > 0)
		ewl_text_length_maximum_set(EWL_TEXT(entry), str->maxlen);
	if (str->str && str->str[0])
		ewl_text_text_set(EWL_TEXT(entry), str->str);
	ewl_container_child_append(EWL_CONTAINER(box), entry);
	ewl_widget_show(entry);

	str->data = entry;
}




static void
gewl_arg_coords(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	Ewl_Widget *box;
	Ewl_Widget **entries;
	struct order_arg_coord *coords;
	const char *labels[] = { "X:", "Y:", "Z:" };
	int i;

	assert(od);
	assert(arg);

	coords = &(orderinfo->coord);

	entries = calloc(3,sizeof(Ewl_Widget *));
	coords->data = entries;

	box = gewl_box_with_label(arg->name, od->argbox);

	for (i = 0 ; i < 3 ; i ++){
		/* FIXME: Convert to spinners */
		box = gewl_box_with_label(labels[i], od->argbox);
		entries[i] = ewl_entry_new();
		ewl_container_child_append(EWL_CONTAINER(box), entries[i]);
		ewl_widget_show(entries[i]);
	}
}

static void 
gewl_arg_turns(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	Ewl_Widget *box;
	Ewl_Widget *entry;
	struct order_arg_time *tm;

	assert(od);
	assert(arg);

	tm = &orderinfo->time;

	box = gewl_box_with_label(arg->name, od->argbox);

	entry = ewl_spinner_new();
	ewl_range_maximum_value_set(EWL_RANGE(entry), tm->max);
	ewl_range_minimum_value_set(EWL_RANGE(entry), tm->max);
	ewl_range_step_set(EWL_RANGE(entry), 1);
	ewl_spinner_digits_set(EWL_SPINNER(entry), log(tm->max)/log(10));
	ewl_container_child_append(EWL_CONTAINER(box), entry);
	ewl_widget_show(entry);
}
static void 
gewl_arg_object(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	Ewl_Widget *box;
	Ewl_Widget *entry;
	struct order_arg_object *objectarg;

	assert(od);
	assert(arg);

	objectarg = &(orderinfo->object);

	box = gewl_box_with_label(arg->name, od->argbox);

	/* FIXME: Should use a drop down or similar */
	entry = ewl_entry_new();
	ewl_container_child_append(EWL_CONTAINER(box), entry);
	ewl_widget_show(entry);

	objectarg->data = entry;
}

static void
gewl_arg_player(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	Ewl_Widget *box;
	Ewl_Widget *entry;

	assert(od);
	assert(arg);

	box = gewl_box_with_label(arg->name, od->argbox);

	/* FIXME: Should use a drop down or similar */
	/* FIXME: need to generate a list using filter */
	entry = ewl_entry_new();
	ewl_container_child_append(EWL_CONTAINER(box), entry);
	ewl_widget_show(entry);

}
static void
gewl_arg_relcoords(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	Ewl_Widget **entries;
	Ewl_Widget *box;
	struct order_arg_relcoord *coord;

	assert(od);
	assert(arg);

	entries = calloc(4,sizeof(Ewl_Widget*));
	coord = &(orderinfo->relcoord);
	coord->data = entries;

	box = gewl_box_with_label(arg->name, od->argbox);

	/* FIXME: Should use a drop down or similar */
	box = gewl_box_with_label("To:", od->argbox);
	entries[0] = ewl_entry_new();
	//if (arg->max > 0)
	//	ewl_text_length_maximum_set(EWL_TEXT(entry), arg->max);
	ewl_container_child_append(EWL_CONTAINER(box), entries[0]);
	ewl_widget_show(entries[0]);

	box = gewl_box_with_label("X:", od->argbox);
	entries[1] = ewl_entry_new();
	ewl_container_child_append(EWL_CONTAINER(box), entries[1]);
	ewl_widget_show(entries[1]);

	box = gewl_box_with_label("Y:", od->argbox);
	entries[2] = ewl_entry_new();
	ewl_container_child_append(EWL_CONTAINER(box), entries[2]);
	ewl_widget_show(entries[2]);

	box = gewl_box_with_label("Z:", od->argbox);
	entries[3] = ewl_entry_new();
	ewl_container_child_append(EWL_CONTAINER(box), entries[3]);
	ewl_widget_show(entries[3]);

}
static void 
gewl_arg_range(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	Ewl_Widget *box;
	Ewl_Widget *entry;
	struct order_arg_range *range;

	box = gewl_box_with_label(arg->name, od->argbox);

	range = &orderinfo->range;

	entry = ewl_spinner_new();
	ewl_range_minimum_value_set(EWL_RANGE(entry),range->min);
	ewl_range_maximum_value_set(EWL_RANGE(entry),range->max);
	ewl_range_step_set(EWL_RANGE(entry), range->inc);
	ewl_spinner_digits_set(EWL_SPINNER(entry), 0);
	ewl_container_child_append(EWL_CONTAINER(box), entry);
	ewl_widget_show(entry);

	range->data = entry;

}
static void
gewl_arg_list(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	Ewl_Widget *button;
	Ewl_Widget *box;
	Ewl_Widget *row;
	struct order_arg_list *list;
	struct gewl_arg_list *gal;

	gewl_box_with_label(arg->name, od->argbox);

	list = &(orderinfo->list);

	gal = calloc(1,sizeof(struct gewl_arg_list));
	gal->list = list;
	gal->box = od->argbox;
	list->data = gal;

	box = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(od->argbox), box);
	ewl_widget_show(box);

	gal->model = ewl_model_new();
	ewl_model_data_fetch_set(gal->model, gewl_arg_list_data_fetch);
	ewl_model_data_count_set(gal->model, gewl_arg_list_count_get);

	/* create the view for ewl_label widgets */
	gal->view = ewl_view_clone(ewl_label_view_get());
	ewl_view_header_fetch_set(gal->view, gewl_arg_list_header_fetch);

	row = gewl_arg_list_row_add(gal);
	ewl_container_child_append(EWL_CONTAINER(gal->box),row);

	button = ewl_button_new();
	ewl_button_label_set(EWL_BUTTON(button), "Another");
	ewl_container_child_append(EWL_CONTAINER(od->argbox), button);
	ewl_object_fill_policy_set(EWL_OBJECT(button), EWL_FLAG_ALIGN_RIGHT | 
			EWL_FLAG_FILL_HSHRINK);
	ewl_callback_append(button, EWL_CALLBACK_CLICKED,
			gewl_arg_list_add_row_cb, gal);	
	ewl_widget_show(button);
}

static void
gewl_arg_list_add_row_cb(Ewl_Widget *w, void *data, void *galv){
	struct gewl_arg_list *gal = galv;
	Ewl_Widget *row;
	int i;
	
	row = gewl_arg_list_row_add(gal);

	/* FIXME: This doesn't seem to work */
	i = ewl_container_child_index_get(EWL_CONTAINER(gal->box), w);
	ewl_container_child_insert(EWL_CONTAINER(gal->box), row,0);

}

static Ewl_Widget *
gewl_arg_list_row_add(struct gewl_arg_list *gal){
	struct gewl_arg_list_selections *sel;
	Ewl_Widget *combo;
	Ewl_Widget *spinner;
	Ewl_Widget *label;
	Ewl_Widget *box;

	box = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(gal->box), box);
	ewl_widget_show(box);

	combo = ewl_combo_new();
	ewl_container_child_append(EWL_CONTAINER(box), combo);
	ewl_mvc_model_set(EWL_MVC(combo), gal->model);
	ewl_mvc_view_set(EWL_MVC(combo), gal->view);
	ewl_mvc_data_set(EWL_MVC(combo), gal->list);
	ewl_widget_show(combo);

	label = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(label), "x");
	ewl_container_child_append(EWL_CONTAINER(box), label);
	ewl_widget_show(label);

	spinner = ewl_spinner_new();
	ewl_range_step_set(EWL_RANGE(spinner),1);
	ewl_range_minimum_value_set(EWL_RANGE(spinner), 0);
	ewl_range_maximum_value_set(EWL_RANGE(spinner), 1);
	ewl_spinner_digits_set(EWL_SPINNER(spinner), 0);
	ewl_container_child_append(EWL_CONTAINER(box), spinner);
	ewl_widget_show(spinner);

	/* Now we have a spinner, associate it to the combo callback */
	ewl_callback_append(combo, EWL_CALLBACK_VALUE_CHANGED,
					gewl_arg_list_value_changed, spinner);

	sel = calloc(1,sizeof(struct gewl_arg_list));
	sel->spinner = spinner;
	sel->combo = combo;
	sel->next = gal->selections;
	gal->selections = sel;

	return box;
}

static Ewl_Widget *
gewl_arg_list_header_fetch(void *data, unsigned int col){
	Ewl_Widget *header;

	header = ewl_label_new();
	/* FIXME: Use label from data */
	ewl_label_text_set(EWL_LABEL(header), "Option");
	ewl_widget_show(header);

	return header;
}

static void *
gewl_arg_list_data_fetch(void *data, unsigned int row, 
		unsigned int col){
	struct order_arg_list *ol;

	ol = data;

	if (row > ol->noptions)
		return NULL;
	else
		return ol->options[row].option;
}

static unsigned int 
gewl_arg_list_count_get(void *data){
	struct order_arg_list *ol;

	ol = data;
	return ol->noptions;
}

static void
gewl_arg_list_value_changed(Ewl_Widget *widget, void *event, void *data){
	struct order_arg_list *oi;
	Ewl_Selection_Idx *index;
	Ewl_Widget *spinner;

	spinner = data;
	oi = ewl_mvc_data_get(EWL_MVC(widget));
	index = ewl_mvc_selected_get(EWL_MVC(widget));
	if (index == NULL){
		printf("No selected item\n");
		/* FIXME: make spinner insensitve */
		ewl_widget_disable(spinner);
		return;
	} 

	ewl_widget_enable(spinner);

	ewl_range_maximum_value_set(EWL_RANGE(spinner), 
			oi->options[index->row].max);

	printf("Index is %d [%d %s max: %d]\n",index->row,
			oi->options[index->row].id,
			oi->options[index->row].option,
			oi->options[index->row].max);

}


static void
gewl_arg_ref(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	printf("Arg Reference not handled\n");
}

static void
gewl_arg_reflist(struct ewl_order_data *od, struct order_arg *arg,
		union order_arg_data *orderinfo){
	printf("Arg Reference list not handled\n");
}


static Ewl_Widget *
gewl_box_with_label(const char *str, Ewl_Widget *parent){
	Ewl_Widget *label;
	Ewl_Widget *box;

	box = ewl_vbox_new();
	ewl_container_child_append(EWL_CONTAINER(parent), box);
	ewl_widget_show(box);

	label = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(label), str);
	ewl_container_child_append(EWL_CONTAINER(box), label);
	ewl_widget_show(label);

	return box;
}

static int
coords_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	struct order_arg_coord *coords;
	int i;
	Ewl_Widget **entries;

	coords = &(argdata->coord);

	assert(coords->data);
	entries = coords->data;

	coords->x = ewl_range_value_get(EWL_RANGE(entries[0]));
	coords->y = ewl_range_value_get(EWL_RANGE(entries[1]));
	coords->z = ewl_range_value_get(EWL_RANGE(entries[2]));

	for (i = 0 ; i < 3 ; i ++)
		ewl_widget_destroy(entries[i]);
	free(entries);
	coords->data = NULL;

	return 0;
}
static int
turn_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	struct order_arg_time *tm;
	Ewl_Widget *spinner;

	tm = &(argdata->time);

	assert(tm->data);
	spinner = tm->data;

	tm->turns = ewl_range_value_get(EWL_RANGE(spinner));

	ewl_widget_destroy(spinner);
	tm->data = NULL;
	return 0;
}
static int
object_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	struct order_arg_object *argobject;
	Ewl_Widget *entry;
	const char *objid;

	argobject = &(argdata->object);
	
	entry = argobject->data;
	
	objid = ewl_text_text_get(EWL_TEXT(entry));

	argobject->oid = strtol(objid,NULL,0);

	argobject->data = NULL;

	return 0;
}
static int
player_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	struct order_arg_player *player;
	Ewl_Widget *combo;
	//const char *playername;
	Ewl_Selection_Idx *index;

	player = &(argdata->player);

	combo = player->data;

	index = ewl_mvc_selected_get(EWL_MVC(combo));
	if (index == NULL){
		printf("No selected item\n");
		return -1;
	}
	
	/* FIXME: This is broken */
	player->pid = index->row;


	return 1;
}
static int
relc_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	struct order_arg_relcoord *coords;
	int i;
	Ewl_Widget **entries;
	const char *objid;

	coords = &(argdata->relcoord);

	assert(coords->data);
	entries = coords->data;

	/* FIXME: Should be a drop down of some sort */
	objid = ewl_text_text_get(EWL_TEXT(entries[0]));
	coords->obj = strtol(objid, NULL, 0);

	coords->x = ewl_range_value_get(EWL_RANGE(entries[1]));
	coords->y = ewl_range_value_get(EWL_RANGE(entries[2]));
	coords->z = ewl_range_value_get(EWL_RANGE(entries[3]));

	for (i = 0 ; i < 3 ; i ++)
		ewl_widget_destroy(entries[i]);
	free(entries);
	coords->data = NULL;

	return 0;
}

static int
range_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	struct order_arg_range *range;

	range = &(argdata->range);
	range->value = ewl_range_value_get(EWL_RANGE(range->data));

	return 0;
}
static int
list_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	struct order_arg_list *list;
	struct gewl_arg_list *gal;
	struct gewl_arg_list_selections *sel;
	struct order_arg_list_selection *argsels;
	int i,n;

	list = &(argdata->list);
	gal = list->data;

	for (sel = gal->selections, n = 0 ; sel ; sel =sel->next){
		n ++;
	}

	argsels = calloc(n, sizeof(struct order_arg_list_selection));
	list->nselections = n;
	list->selections = argsels;

	for (i = 0 , sel = gal->selections ; sel ; sel = sel->next, i ++){
		Ewl_Selection_Idx *idx;
	
		argsels[i].count = ewl_range_value_get(EWL_RANGE(sel->spinner));
		idx = ewl_mvc_selected_get(EWL_MVC(sel->combo));
		argsels[i].selection = list->options[idx->row].id;
	}
	
	return 0;
}
static int
string_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *orderinfo){
	struct order_arg_string *str;

	str = &(orderinfo->string);

	if (str->str) free(str->str);
	str->str = ewl_text_text_get(EWL_TEXT(str->data));
	ewl_widget_destroy(str->data);
	
	return 0;
}
static int
ref_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	return 1;
}
static int
refl_save(struct ewl_order_data *id, struct order_arg *arg,
		union order_arg_data *argdata){
	return 1;
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
