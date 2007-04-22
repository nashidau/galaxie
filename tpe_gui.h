struct tpe;
struct gui;
struct gui_obj; /* Data for a object */
struct gui *gui_init(struct tpe *tpe, const char *theme, unsigned int fullscreen);

struct gui_obj *gui_object_data_get(struct gui *gui, struct object *);
	
