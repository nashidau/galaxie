#include <string.h>


#include "test.h"

#include "star.h"

static struct teststars {
	objid_t id;
	const char *name;
} teststars[] = {
	{ 27,	"Elsinore" },
};
#define N_TESTSTARS ((int)(sizeof(teststars)/sizeof(teststars[0])))

int
main(int argc, char **argv){
	Evas_Object *star;
	struct test *test;
	const char *name;
	objid_t id;

	test = testinit(&argc, argv);
	if (!test){
		printf("Unable to init Test system\n");
		exit(1);
	}
	
	star = gui_star_add(test->e, teststars[0].id, teststars[0].name);
	evas_object_move(star,10,10);
	evas_object_show(star);

	name = gui_star_name_get(star);
	id = gui_star_id_get(star);

	printf("Star name is: %s\n", name);
	if (!name || strcmp(name, teststars[0].name) != 0)
		printf("Should be '%s'\n", teststars[0].name);

	printf("Id is %d\n", id);
	if (id != teststars[0].id)
		printf("Should be '%d'\n", teststars[0].id);

	ecore_main_loop_begin();

	return 0;
}
