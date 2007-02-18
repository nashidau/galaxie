struct tpe;
struct tpe_obj;
struct object;

enum objtype {
	OBJTYPE_UNIVERSE,
	OBJTYPE_GALAXY,
	OBJTYPE_SYSTEM, /* Star System */
	OBJTYPE_PLANET, /* Planet */
	OBJTYPE_FLEET, 
};

struct vector {
	uint64_t x,y,z;
};

/* Represents an object in the system */
struct object {
	uint32_t oid;	/* Unique ID */
	enum objtype type;
	char *name;
	uint64_t size;
	struct vector pos;
	struct vector vel;

	int nchildren;
	int *children;

	int nordertypes;
	int *ordertypes;

	int norders;

	uint64_t updated;

	Evas_Object *obj;
};


struct tpe_obj * tpe_obj_init(struct tpe *tpe);
int tpe_obj_obj_dump(struct object *o);
struct object *tpe_obj_obj_get_by_id(struct tpe_obj *obj, int oid);
struct object *tpe_obj_obj_add(struct tpe_obj *obj, int );


