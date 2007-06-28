#include <Ewl.h> /* FIXME */

int gui_window_add(struct gui *gui, Evas_Object *window);
Evas_Object *gui_window_add2(struct gui *gui, const char *title, const char *icon);

int gui_window_focus(struct gui *gui, Evas_Object *window);
Ewl_Widget *gui_window_ewl_add(struct gui *gui);
