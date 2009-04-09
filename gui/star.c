/**
 * @brief Basic Star Widget.
 *
 * The star widget is a single system on the map.  In some games it is the
 * basic item for navigation, in others it will contain planets, asteroids and
 * other things. 
 *
 * It can also act as a container to hold splanets, ships and other widgets.
 *
 * Mouse/Cursor behaviour (default):
 *  - Mouse in:
 *		Generate GUIObjectHighlight
 *	Wait 1s:
 *		Generate GUIObjectSelect
 *  - Mouse out 
 *		Nothing (cancel possible GUIObjectSelect event)
 *  - Mouse up:
 *		Generate GUIObjectSelect
 *
 * @todo: Doxygen Documentation
 * @todo: Asynchronous media loading
 * @todo: TP Media support
 * @todo: Event handling
 * @todo: Event generation
 * @todo: Label toggle
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>

/* FIXME: Generic types */
#include "../object_param.h"
#include "../tpe_event.h"

#include "star.h"
#include "widgetsupport.h"

#ifndef DATAPATH
#warning No datapath - using default
#define DATAPATH "/usr/local/share/galaxie/"
#endif

enum {
	DEFAULT_SIZE = 32
};

#define INSELECT 1.0

/* FIXME: This should be in globals somewhere */
static const objid_t INVALIDID = 0xffffffff;

typedef struct Smart_Data {

	Evas *e;

	Evas_Coord x,y;

	objid_t id;	

	Evas_Object *clip;
	Evas_Object *star;
	Evas_Object *name;

	Ecore_Timer *intimer;

	unsigned int defaultname : 1;
} Smart_Data;

static char * start_default_name_get(objid_t id);

static const char classname[] = "Star";

static void smart_add(Evas_Object *);
static void smart_del(Evas_Object *);
static void smart_move(Evas_Object *, Evas_Coord x, Evas_Coord y);
static void smart_member_add(Evas_Object *, Evas_Object *);
static void smart_member_del(Evas_Object *, Evas_Object *);

static void smart_mouse_up(void *data, Evas *, Evas_Object *, void *ev);
static void smart_mouse_down(void *data, Evas *, Evas_Object *, void *ev);
static void smart_mouse_in(void *data, Evas *, Evas_Object *, void *ev);
static void smart_mouse_out(void *data, Evas *, Evas_Object *, void *ev);

static int smart_mouse_hover(void *starv);

static const Evas_Smart_Class starclass = {
	.name = classname,
	.version = EVAS_SMART_CLASS_VERSION,
	.add = smart_add,
	.del = smart_del,
	.move = smart_move,
	.member_add = smart_member_add,
	.member_del = smart_member_del,
};
static Evas_Smart *smartclass;

Evas_Object *
gui_star_add(Evas *e, objid_t id, const char *name){
	Evas_Object *star;

	if (!e) return NULL;

	if (!smartclass)
		smartclass = evas_smart_class_new(&starclass);	

	star = evas_object_smart_add(e, smartclass);
	
	if (name)
		gui_star_name_set(star,name);
	gui_star_id_set(star,id);

	return star;
}


/** 
 * Sets the name of the star. 
 *
 * Causes a resize if necessary.
 *
 * @param obj Star widget.
 * @param name New star name.
 * @return 0 on success, -1 on error.
 */
int 
gui_star_name_set(Evas_Object *obj, const char *name){
	Smart_Data *sd;
	
	sd = CHECK_TYPE_RETURN(obj, "Star", -1);

	evas_object_text_text_set(sd->name, name);
	sd->defaultname = 0; /* Manually set */
	/* FIXME: Resize here */

	return 0;
}

/**
 * Gets the current star name.
 *
 * The name must not be freed (or modfied) by the caller.
 * Note that if no name has been set it returns a default name.
 *
 * @param obj Star widget.
 * @return Star name, or NULL on error.
 */
const char *
gui_star_name_get(Evas_Object *obj){
	Smart_Data *sd;

	sd = CHECK_TYPE_RETURN(obj, "Star", NULL);
	return evas_object_text_text_get(sd->name);
}

/**
 * Sets the star id.
 *
 * If the star is using a default name this will also change the name.
 *
 * @param star The star.
 * @param id The new id.
 * @return 0 on success, -1 on error.
 */
int 
gui_star_id_set(Evas_Object *star, objid_t id){
	Smart_Data *sd;
	char *name;
	
	sd = CHECK_TYPE_RETURN(star, "Star", -1);

	if (id == sd->id) return 0;
	
	if (sd->defaultname){
		name = start_default_name_get(id);
		gui_star_name_set(star,name);
		sd->defaultname = 1; /* Set it back */
		free(name);
	}

	sd->id = id;
	return 0;
}

/**
 * Gets the object ID associated with this star.
 *
 * Note that the error value and the undefined value are the same.
 *
 * @param star The star.
 * @return Object ID or INVALIDID (0xffffffff) on error.
 */
objid_t
gui_star_id_get(Evas_Object *star){
	Smart_Data *sd;
	
	sd = CHECK_TYPE_RETURN(star, "Star", INVALIDID);	

	return sd->id;
}


static void 
smart_add(Evas_Object *star){
	Smart_Data *sd;
	int err;

	sd = calloc(1,sizeof(struct Smart_Data));

	evas_object_smart_data_set(star,sd);

	sd = CHECK_TYPE(star, classname);
	sd->e = evas_object_evas_get(star);

	sd->defaultname = 1;

	sd->clip = evas_object_rectangle_add(sd->e);

	sd->star = evas_object_image_add(sd->e);
	evas_object_image_file_set(sd->star, DATAPATH"/widgets/star.png",NULL);
	err = evas_object_image_load_error_get(sd->star);
	if (err != 0){
		fprintf(stderr, "Unable to load star image (%d)\n",err);
		fprintf(stderr,"Was loading: %s\n",DATAPATH"widgets/star.png");
	}
	evas_object_show(sd->star);
	evas_object_resize(sd->star,DEFAULT_SIZE,DEFAULT_SIZE);
	evas_object_image_fill_set(sd->star,0,0,DEFAULT_SIZE,DEFAULT_SIZE);
	evas_object_smart_member_add(sd->star,star);
	evas_object_event_callback_add(sd->star, EVAS_CALLBACK_MOUSE_DOWN,
				smart_mouse_down, star);
	evas_object_event_callback_add(sd->star, EVAS_CALLBACK_MOUSE_UP,
				smart_mouse_up, star);
	evas_object_event_callback_add(sd->star, EVAS_CALLBACK_MOUSE_IN,
				smart_mouse_in, star);
	evas_object_event_callback_add(sd->star, EVAS_CALLBACK_MOUSE_OUT,
				smart_mouse_out, star);

	sd->name = evas_object_text_add(sd->e);
	evas_object_text_style_set(sd->name, EVAS_TEXT_STYLE_SHADOW);
	evas_object_text_shadow_color_set(sd->name, 50,128,60,128);
	/* FIXME: Style */
	evas_object_text_font_set(sd->name, "Vera", 12);
	evas_object_show(sd->name);
	evas_object_smart_member_add(sd->name,star);

}
static void 
smart_del(Evas_Object *star){
	Smart_Data *sd;

	sd = CHECK_TYPE(star, classname);

	evas_object_del(sd->clip);
	evas_object_del(sd->star);
	evas_object_del(sd->name);
}

static void 
smart_member_add(Evas_Object *star, Evas_Object *child){
	

}
static void 
smart_member_del(Evas_Object *star, Evas_Object *child){

}

static void
smart_move(Evas_Object *star, Evas_Coord x, Evas_Coord y){
	Smart_Data *sd;
	int sw,sh,tw;

	sd = CHECK_TYPE(star, classname);
	
	if (sd->x == x && sd->y == y) return;

	evas_object_move(sd->clip,x,y);
	evas_object_move(sd->star,x,y);
	
	evas_object_geometry_get(sd->star,NULL,NULL,&sh,&sw);
	evas_object_geometry_get(sd->name,NULL,NULL,&tw,NULL);
	evas_object_move(sd->name,x + sw / 2 - tw / 2, y + sh);

	sd->x = x;
	sd->y = y;
}


static void 
smart_mouse_up(void *data, Evas *e, Evas_Object *obj, void *ev){

}
static void 
smart_mouse_down(void *data, Evas *e, Evas_Object *obj, void *ev){
}
static void 
smart_mouse_in(void *starv, Evas *e, Evas_Object *obj, void *ev){
	Smart_Data *sd;

	sd = CHECK_TYPE(starv, classname);

	if (sd->intimer){
		fprintf(stderr,"%s:%d: In on in?\n",__FILE__,__LINE__);
		ecore_timer_del(sd->intimer);
	}
	sd->intimer = ecore_timer_add(INSELECT, smart_mouse_hover, starv);

	/* FIXME: Hard coded string for event type */
	tpe_event_send("GUIObjectHover",INTTOPTR(sd->id),tpe_event_nofree,NULL);

}
static void
smart_mouse_out(void *starv, Evas *e, Evas_Object *obj, void *ev){
	Smart_Data *sd;

	sd = CHECK_TYPE(starv, classname);

	if (sd->intimer){
		ecore_timer_del(sd->intimer);
		sd->intimer = NULL;
	}

}


static int 
smart_mouse_hover(void *starv){
	Smart_Data *sd;

	sd = CHECK_TYPE_RETURN(starv, classname, 0);

	sd->intimer = NULL;
	return 0;
}


/**
 * Generate a default name for a star.
 */
static char *
start_default_name_get(objid_t id){
	char buf[100];
	if (id != INVALIDID)
		snprintf(buf,sizeof(buf), "Star #%d",id);
	else
		strncpy(buf,"Planet X", sizeof(buf));
	return strdup(buf);	
}
