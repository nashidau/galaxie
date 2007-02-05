#include <stdlib.h>
#include <stdio.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Edje.h>

#include "tpe.h"
#include "tpe_comm.h"

struct tpe_gui {
	Ecore_Evas  *ee;
	Evas        *e;
	Evas_Object *connect;

	Evas_Object *main;

	Evas_Object *background;
};

enum {
	WIDTH = 1024,
	HEIGHT = 768
};

#define URI	"localhost"	/* FIXME */

static void tpe_gui_edje_splash_connect(void *data, Evas_Object *o, 
		const char *emission, const char *source);

struct tpe_gui *
tpe_gui_init(struct tpe *tpe){
	struct tpe_gui *gui;

	gui = calloc(1,sizeof(struct tpe_gui));
	
	/* FIXME: Should check for --nogui option */

	gui->ee = ecore_evas_software_x11_new(NULL, 0,  0, 0, WIDTH, HEIGHT);
	if (gui->ee == NULL) return 0;
	ecore_evas_title_set(gui->ee, "Ecore Template");
	ecore_evas_borderless_set(gui->ee, 0);
	ecore_evas_show(gui->ee);
	gui->e = ecore_evas_get(gui->ee);

	evas_font_path_append(gui->e, "data/");

	edje_init();

	gui->background = edje_object_add(gui->e);
	edje_object_file_set(gui->background,"edje/background.edj", 
				"Background");
	evas_object_move(gui->background, 0,0);
	evas_object_show(gui->background);
	evas_object_layer_set(gui->background,-100);
	evas_object_resize(gui->background,WIDTH,HEIGHT);

	gui->connect = edje_object_add(gui->e);
	edje_object_file_set(gui->connect,"edje/basic.edj", "Splash");
	evas_object_move(gui->connect,0,0);
	evas_object_show(gui->connect);
	evas_object_resize(gui->connect,WIDTH,HEIGHT);
	edje_object_signal_callback_add(gui->connect, "Splash.connect", "",
			tpe_gui_edje_splash_connect, tpe);


	return gui;
}


/**
 * Callback from the splash screen to start a connection
 */
static void 
tpe_gui_edje_splash_connect(void *data, Evas_Object *o, 
		const char *emission, const char *source){
	struct tpe *tpe;

	tpe = data;

	tpe_comm_connect(tpe->comm, "localhost", 6923, "nash", "password");

	/* General errors */
/*	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_FAIL,
			tpe_fail, tpe);
*/
	/* Feature frames */
	/*
	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_AVAILABLE_FEATURES, 
			tpe_features, tpe);
	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_TIME_REMAINING, 
			tpe_time_remaining, tpe);
	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_LIST_OF_BOARDS, tpe_board, tpe);
	tpe_msg_event_handler_add(tpe->msg, TPE_MSG_LIST_OF_RESOURCES_IDS,
			tpe_resource_description, tpe);
	*/	

	/* Jump to the map screen */

}



	

