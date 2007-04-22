enum board_state {
	BOARD_UNREAD,
	BOARD_READ,
};

struct gui {
	struct tpe *tpe;

	Ecore_Evas  *ee;
	Evas        *e;

	Evas_Object *main;

	Evas_Object *background;

	struct {
		uint64_t zoom;
		int top,left;
		int w,h;
	} map;

	/* Bounding box of the universe */
	struct {
		int64_t minx,miny;
		int64_t maxx,maxy;
	} bb;

	/* For mouse overs - may be more then one if they have a special
	 * delete */
	Evas_Object *popup;

	/* Board popups */
	Evas_Object *boardpopup;

	Ecore_List *visible;

	Ecore_List *boards;

	/* If set will take a screen shot every turn */
	int record;

	Ecore_List *windows;
};

struct gui_board {
	struct gui *gui;
	Evas_Object *obj;
	uint32_t boardid;
	int state;
	const char *name;
	const char *desc;

};


/**
 * Gui data attached to an object
 *
 */
struct gui_obj {
	Evas_Object *obj;

	struct gui *gui;
	struct object *object;

	int nplanets;

	/* The info window (if any) for this object */
	Evas_Object *info;

	/* Order window - if any */
	Evas_Object *orders;

	/* Last visible state */
	const char *state;

};


