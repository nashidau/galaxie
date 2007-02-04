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

#define WIDTH	1024
#define HEIGHT	768

struct features {
	int id;
	const char *desc;
} features[] =  {
	{ 1,	"SSL connection on this port" },
	{ 2,	"SSL connection on another port" },
	{ 3,	"http connect on this port" },
	{ 4,	"http connect on another port" },
	{ 5,	"keep alive" },
	{ 6,	"Serverside properties" },
	{ 1000,	"Automatic account registration" },
};
#define N_FEATURES (sizeof(features)/sizeof(features[0]))


/* FIXME: These need to be sp;lit into different files */
static int tpe_features(void *data, int type, void *event);
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

	ecore_main_loop_begin();

	return 0;
}

static int 
tpe_features(void *udata, int type, void *event){
	uint32_t *data;
	int i,j,len,feature;

	data = (uint32_t *)event + 4;
	len = ntohl(*data);
	if (len > 32) len = 32;

	if ((len + 1) * 4!= ntohl(data[-1]))
		printf("Data lengths don't match: %d items vs %d bytes\n",
				len, ntohl(data[-1]));

	for (i = 0 ; i < len ; i ++){
		feature = ntohl(data[i + 1]);
		for (j = 0 ; j < N_FEATURES ; j ++)
			if (features[j].id == feature)
				printf("\tFeature: %s\n",features[j].desc);
	}

	return 1;

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






