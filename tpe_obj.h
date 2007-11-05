struct tpe;
struct tpe_obj;
struct tpe_gui_obj;
struct object;

enum {
	TPE_OBJ_NIL = -1,
};

enum objtype {
	OBJTYPE_UNIVERSE,
	OBJTYPE_GALAXY,
	OBJTYPE_SYSTEM, /* Star System */
	OBJTYPE_PLANET, /* Planet */
	OBJTYPE_FLEET, 
};

struct vector {
	int64_t x,y,z;
};

struct planet_resource {
	uint32_t rid;
	uint32_t surface;
	uint32_t minable;
	uint32_t inaccessable;
};

struct object_planet {
	int nresources;
	struct planet_resource *resources;
};

struct fleet_ship {
	uint32_t design;
	uint32_t count;
};

struct object_fleet {
	int nships;
	struct fleet_ship *ships;
	uint32_t damage;
};

/* Represents an object in the system */
struct object {
	const char *magic;

	struct tpe *tpe;
	struct server *server;

	uint32_t oid;	/* Unique ID */
	enum objtype type;
	char *name;
	char *description;

	int32_t owner;

	uint64_t size;
	struct vector pos;
	struct vector vel;

	int parent; 
	int nchildren;
	int *children;

	int nordertypes;
	int *ordertypes;

	int norders;
	struct order **orders;

	uint64_t updated;

	struct object_fleet *fleet;
	struct object_planet *planet;

	struct gui_obj *gui;
	struct ai_obj *ai;

	int ref;

	unsigned int isnew : 1;
	unsigned int changed : 1;
};

/* Cost to build something : From orders */
struct build_resources {
	int rid;
	int cost;
};

extern const char *const object_magic;

#ifndef OBJECT_MAGIC_CHECK
#define OBJECT_MAGIC_CHECK(object)	\
	do {							\
		if (object == NULL) {				\
			fprintf(stderr,"Object is NULL");	\
			return;					\
		}						\
		if (object->magic == object_magic){		\
			fprintf(stderr,"Incorrect object magic"); \
			return;					\
		}						\
	}
#endif


struct tpe_obj * tpe_obj_init(struct tpe *tpe);
int tpe_obj_obj_dump(struct object *o);
struct object *tpe_obj_obj_get_by_id(struct tpe *tpe, uint32_t oid);
struct object *tpe_obj_obj_add(struct tpe_obj *obj, int );

struct object *tpe_obj_obj_sibling_get(struct tpe *tpe, struct object *, int prev);
struct object *tpe_obj_obj_child_get(struct tpe *tpe, struct object *);
struct object *tpe_obj_obj_parent_get(struct tpe *tpe, struct object *);

uint64_t tpe_obj_object_updated(struct tpe *, uint32_t oid);

/* Really for the AI use only... */
Ecore_List *tpe_obj_obj_list(struct tpe_obj *obj);

Ecore_List *tpe_obj_obj_list_by_type(struct tpe *tpe, enum objtype type);

struct object *tpe_obj_home_get(struct tpe *tpe);
