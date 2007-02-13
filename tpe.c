#include <stdio.h>
#include <stdlib.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Con.h>
#include <Edje.h>

#include "tpe.h"
#include "tpe_event.h"
#include "tpe_comm.h"
#include "tpe_msg.h"
#include "tpe_gui.h"
#include "tpe_obj.h"

#define WIDTH	1024
#define HEIGHT	768


/* FIXME: These need to be sp;lit into different files */
static int tpe_time_remaining(void *data, int type, void *event);
static int tpe_board(void *data, int type, void *event);
static int tpe_resource_description(void *data, int type, void *event);
static int tpe_fail(void *data, int type, void *event);

int
main(int argc, char **argv){
	struct tpe *tpe;

	evas_init();
	ecore_init();

	tpe = calloc(1,sizeof(struct tpe));
	if (tpe == NULL) exit(1);

	tpe->event = tpe_event_init(tpe);
	tpe->msg   = tpe_msg_init(tpe);
	tpe->gui   = tpe_gui_init(tpe);
	tpe->comm  = tpe_comm_init(tpe);
	tpe->obj   = tpe_obj_init(tpe);

	ecore_main_loop_begin();

	return 0;
}


static int 
tpe_time_remaining(void *data, int type, void *event){
	printf("Time remaining: %d\n", ntohl(*((int32_t*)event + 4)));
	return 11;
}
static int 
tpe_board(void *data, int type, void *event){
	return 1;

}
static int 
tpe_resource_description(void *data, int type, void *event){
	return 1;
}

static int
tpe_fail(void *data, int type, void *event){
	int len;

	len = ntohl((*((int32_t*)event + 4)));
	printf("Error on message seq %d: %s\n",ntohl(*((int32_t*)event + 1)),
			(char *)event + 20);

	return 1;
}






