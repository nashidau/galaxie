#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Eina.h>

/* For the types:
	FIXME: galaxie/types.h or smilar */
#include "../object_param.h"

#include "../tpe_event.h"

enum {
	TEST_W	= 640,
	TEST_H  = 480
};

struct test {
	Ecore_Evas *ee;
	Evas *e;
	
	/* Elementary ?? */

	int w,h;

	Evas_Object *bg; 
};

struct test *testinit(int *argc, char **argv);
