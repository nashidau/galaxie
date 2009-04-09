/**
 * @todo: Test star name generation:
 		Create a star, set invalid ID -> name is 'Unknown' 
			       set ID	-> name is 'Star #ID'
			       set ID2  -> name is 'Star #ID2'
			       set name -> name is 'name'
		Create a star, set ID -> name is 'Star #ID'
			tests as above 
 */

#include <string.h>


#include "test.h"

#include "star.h"

static struct teststars {
	objid_t id;
	const char *name;
	int x,y;
	Evas_Object *star;
} teststars[] = {
	{ 27,	"Elsinore",	 10,  10,	NULL },
	{ 87,	"Blarmy",	100,  80,	NULL },
	{ 92,	"Morydore",	133, 220,	NULL },
	{ 110,	"Elgar",	 80, 190,	NULL },
	{ 99,	"Camelot",	203, 100,	NULL },
	{ 12,	"Segnaro",	137, 154,	NULL },
	{ 11,	"Wall",		154,  18,	NULL },
};
#define N_TESTSTARS ((int)(sizeof(teststars)/sizeof(teststars[0])))

int
main(int argc, char **argv){
	Evas_Object *star;
	struct test *test;
	const char *name;
	objid_t id;
	int i;

	test = testinit(&argc, argv);
	if (!test){
		printf("Unable to init Test system\n");
		exit(1);
	}

	for (i = 0 ; i < N_TESTSTARS ; i ++){
		star = gui_star_add(test->e, teststars[i].id, teststars[i].name);
		evas_object_move(star,teststars[i].x,teststars[i].y);
		evas_object_show(star);

		name = gui_star_name_get(star);
		id = gui_star_id_get(star);

		if (!name || strcmp(name, teststars[i].name) != 0)
			printf("Star name: %s should be '%s'\n", 
					name, teststars[i].name);

		if (id != teststars[i].id)
			printf("Should id %d be '%d'\n", id, teststars[i].id);

		teststars[i].star = star;
	}	

	/* Do star ID check again */
	for (i = 0 ; i < N_TESTSTARS ; i ++){
		star = teststars[i].star;

		name = gui_star_name_get(star);
		id = gui_star_id_get(star);

		if (!name || strcmp(name, teststars[i].name) != 0)
			printf("Star name: %s should be '%s'\n", 
					name, teststars[i].name);

		if (id != teststars[i].id)
			printf("Should id %d be '%d'\n", id, teststars[i].id);
	}

	ecore_main_loop_begin();

	return 0;
}
