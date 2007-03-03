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

struct arg_type6 {
	int id;
	char *name;
	int max;
};
struct tpe_orders *tpe_orders_init(struct tpe *tpe);
int tpe_order_get_type_by_name(struct tpe *tpe, const char *name);

int tpe_orders_order_free(struct order *order);
int tpe_orders_order_print(struct tpe *tpe, struct order *order);


int tpe_orders_object_move(struct tpe *tpe, struct object *obj, int slot,
		int64_t x,int64_t y, int64_t z);
int tpe_orders_object_move_object(struct tpe *tpe, struct object *obj, int slot,
		struct object *dest);
int tpe_orders_object_colonise(struct tpe *tpe, struct object *obj, int slot,
		struct object *what);
	

uint64_t tpe_orders_order_desc_updated(struct tpe *tpe, uint32_t id);
struct order_desc *tpe_order_orders_get_desc_by_id(struct tpe *, uint32_t);

