/**
 * General star items.
 *
 * A star is the basic item on the map.
 *
 * Stars generate the following GUI events:
 *  - gui_star_hover (Hover a mouse over a star)
 *  - gui_star_select (Selecting a star)
 *
 * Todo:
 *  - A mechanism to add 'children' 
 *	- Visual indication of things like: 
 *		- Planets
 *		- Ships
 *		- Other items
 * 	- Notifications
 *		- No orders
 *		- Owner change
 *
 * 
 */
Evas_Object *gui_star_add(Evas *, objid_t id, const char *name);

int gui_star_name_set(Evas_Object *, const char *name);
const char *gui_star_name_get(Evas_Object *);

int gui_star_id_set(Evas_Object *, objid_t);
objid_t gui_star_id_get(Evas_Object *);

