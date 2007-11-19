struct order;
struct object;
struct tpe_orders;

enum {
	SLOT_LAST = -1,
};

struct order_arg {
	char *name;
	uint32_t arg_type;
	char *description;
};

struct order_desc {
	uint32_t otype;	 /* Type */
	const char *name;
	const char *description;
	
	int nargs;
	struct order_arg *args; /* The type of the args */

	uint64_t updated;
};

/* Type 0: ARG_COORD */
struct order_arg_coord {
	int64_t x,y,z;
};
/* Type 1: ARG_TIME */
struct order_arg_time {
	uint32_t turns;
	uint32_t max;
};
/* Type 2: ARG_OBJECT */
struct order_arg_object {
	uint32_t oid;
};
/* Type 3: ARG_PLAYER */
struct order_arg_player {
	uint32_t pid;
	uint32_t flags;
};
/* Type 4: ARG_RELCOORD */
struct order_arg_relcoord {
	uint32_t obj;
	int64_t x,y,z;
};

/* Type 5: ARG_RANGE */
struct order_arg_range {
	int32_t value;
	int32_t min,max;
	int32_t inc;
};

/* Type 6: ARG_LIST */
struct order_arg_list_option {
	int id;
	int max;
	char *option;
};
struct order_arg_list_selection {
	uint32_t selection;
	uint32_t count;
};
struct order_arg_list {
	uint32_t noptions; /* # of Options */
	struct order_arg_list_option *options;
	uint32_t nselections;
	struct order_arg_list_selection *selections;
};


/* Type 7: ARG_STRING */ 
struct order_arg_string {
	uint32_t maxlen;
	char *str;
};

/* Type 8: ARG_REFERENCE */
/* FIXME: Not implemented */

union order_arg_data {
	struct order_arg_coord coord;
	struct order_arg_time time;
	struct order_arg_object object;
	struct order_arg_player player;
	struct order_arg_relcoord relcoord;
	struct order_arg_range range;
	struct order_arg_list list;
	struct order_arg_string string;
};



struct order {
	int oid;
	int slot;
	int type;
	int turns;
	int nresources;
	struct build_resources *resources;
	
	/* These come from the order desc */
	union order_arg_data **args;
};



struct arg_type6 {
	int id;
	char *name;
	int max;
};
struct tpe_orders *tpe_orders_init(struct tpe *tpe);
int tpe_order_get_type_by_name(struct tpe *tpe, const char *name);

int tpe_orders_order_free(struct order *order);
int tpe_orders_order_print(struct tpe *tpe, struct order *order);
const char *tpe_orders_str_get(struct tpe *tpe, struct object *obj);
const char *tpe_order_get_name_by_type(struct tpe *tpe, uint32_t type);
const char *tpe_order_get_name(struct tpe *tpe, struct order *order);


int tpe_orders_object_probe(struct tpe *tpe, struct object *obj,uint32_t otype,
		void (*cb)(void *, struct object *, struct order_desc *,
				struct order *), void *udata);

int tpe_orders_object_clear(struct tpe *tpe, struct object *obj);
int tpe_orders_object_move(struct tpe *tpe, struct object *obj, int slot,
		int64_t x,int64_t y, int64_t z);
int tpe_orders_object_move_object(struct tpe *tpe, struct object *obj, int slot,
		struct object *dest);
int tpe_orders_object_colonise(struct tpe *tpe, struct object *obj, int slot,
		struct object *what);
int tpe_orders_object_build(struct tpe *tpe, struct object *obj, int slot,
		const char *name, ...);
	

uint64_t tpe_orders_order_desc_updated(struct tpe *tpe, uint32_t id);
struct order_desc *tpe_order_orders_get_desc_by_id(struct tpe *, uint32_t);

