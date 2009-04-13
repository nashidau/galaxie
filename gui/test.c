
#include "test.h"
#include "widgetsupport.h"

static void register_events(void);

static int event_guiobjecthover(void*,int,void*);
static int event_guiobjectselect(void*,int,void*);

static const struct events {
	const char *name;
	int (*handler)(void *, int, void *);
} events[] = {
	{ "GUIObjectHover", event_guiobjecthover },
	{ "GUIObjectSelect",event_guiobjectselect },
};
#define N_EVENTTYPES ((int)(sizeof(events)/sizeof(events[0])))



/**
 * Generic test support
 *
 */
struct test *
testinit(int *argc, char **argv){
	struct test *test;

	test = calloc(1,sizeof(struct test));
	if (!test) {
		printf("Unable to calloc test structure\n");
		return NULL;
	}

	evas_init();
	ecore_init();
	ecore_evas_init();

	tpe_event_init();
	register_events();

	test->ee = NULL;
	//test->ee = ecore_evas_gl_x11_new(NULL,0,0,0,TEST_W,TEST_H);
	//if (!test->ee)
	//	test->ee = ecore_evas_xrender_xll_new(NULL,0,0,0,TEST_W,TEST_H);
	if (!test->ee)
		test->ee =ecore_evas_software_x11_new(NULL,0,0,0,TEST_W,TEST_H);

	if (!test->ee){
		printf("Unable to init Ecore Evas\n");
		return NULL;
	}

	ecore_evas_title_set(test->ee, "GalaxiE Test");

	test->e = ecore_evas_get(test->ee);
	if (!test->e){
		printf("Unable to get Evas from Ecore Evas\n");
		return NULL;
	}
	ecore_evas_show(test->ee);

	test->bg = evas_object_rectangle_add(test->e);
	evas_object_color_set(test->bg, 0,0,30,255);
	evas_object_move(test->bg,0,0);
	evas_object_resize(test->bg,TEST_W,TEST_H);
	evas_object_layer_set(test->bg, -1000);
	evas_object_show(test->bg);

	test->w = TEST_W;
	test->h = TEST_H;

	return test;	
}


static void
register_events(void){
	int i;

	for (i = 0 ; i < N_EVENTTYPES ; i ++){
		tpe_event_type_add(events[i].name);
		if (events[i].handler)
			tpe_event_handler_add(events[i].name,
					events[i].handler,NULL);
	}
}

static int
event_guiobjecthover(void *data, int type, void *event){
	uint32_t id;

	id = PTRTOINT(event);

	printf("GUIObjectHover generated: Star %d\n",id);
	return 0;
}

static int
event_guiobjectselect(void *data,int type,void *event){
	uint32_t id;

	id = PTRTOINT(event);

	printf("GUIObjectSelect generated: Star %d\n",id);
	return 0;
}
