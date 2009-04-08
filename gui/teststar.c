#include <string.h>


#include "test.h"

#include "star.h"

static struct teststars {
	objid_t id;
	const char *name;
	int x,y;
} teststars[] = {
	{ 27,	"Elsinore",	10, 10 },
	{ 87,	"Blarmy",	100, 80 },
	{ 92,	"Morydore",	133, 220 },
	{ 110,	"Elgar",	80, 190, },
	{ 99,	"Camelot",	203, 100 },
	{ 12,	"Segnaro",	137, 154 },
	{ 11,	"Wall",		154, 18 },
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

		printf("Star name is: %s\n", name);
		if (!name || strcmp(name, teststars[i].name) != 0)
			printf("Should be '%s'\n", teststars[i].name);

		printf("Id is %d\n", id);
		if (id != teststars[i].id)
			printf("Should be '%d'\n", teststars[i].id);
	}	

	ecore_main_loop_begin();

	return 0;
}
