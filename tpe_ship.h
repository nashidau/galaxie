struct tpe_ship;
struct desgin;
struct tpe_ship *tpe_ship_init(struct tpe *);

const char *tpe_ship_design_name_get(struct tpe *, uint32_t design);

uint64_t tpe_ship_design_updated_get(struct tpe *, uint32_t design);
struct design *tpe_ship_design_get(struct tpe *tpe, uint32_t design);
