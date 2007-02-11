/* 
 * General system related functions.
 *
 * Does general stuff related to the system of the game.  Looks after keeping
 * data up to date.
 */

#include "tpe.h"
#include "tpe_sys.h"

struct tpe_sys {
	struct tpe *tpe;
};

static int tpe_sys_object_list(void *data, int eventid, void *event);

struct tpe_sys *
tpe_sys_init(struct tpe *tpe){
	struct tpe_event *event;
	struct tpe_sys *sys;

	event = tpe->event;

	sys = calloc(1,sizeof(struct tpe_sys));
	sys->tpe = tpe;

	tpe_event_handler_add(event, "MsgListOfObjectIDs",
			tpe_sys_object_list, sys);

	return sys;
}

static int 
tpe_sys_object_list(void *data, int eventid, void *event){

}
