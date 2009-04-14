/**
 * Map test program for galaxie.
 */

#include <string.h>


#include "test.h"

#include "map.h"
#include "widgetsupport.h"

int
main(int argc, char **argv){
	Evas_Object *map;
	struct test *test;

	test = testinit(&argc, argv);
	if (!test){
		printf("Unable to init Test system\n");
		exit(1);
	}

	map = gui_map_add(test->e, -1);
	if (!map){
		fprintf(stderr,"Unable to allocate map widget\n");
		exit(1);
	}

	evas_object_move(map,0,0);
	evas_object_resize(map,test->w,test->h);

	/* Start adding stars by event */

	


	ecore_main_loop_begin();

	return 0;
}


