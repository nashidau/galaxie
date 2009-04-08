
#include "test.h"

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

	return test;	
}
