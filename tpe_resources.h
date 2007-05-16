struct tpe_resources;
struct resourcedescription;

struct resourcedescription {
        uint32_t id;
	char *name;
	char *name_plural;
	char *unit;
	char *unit_plural;
	char *description;
	uint32_t weight;
	uint32_t size;
        uint64_t updated;
};



struct tpe_resources *tpe_resources_init(struct tpe *tpe);
struct resourcedescription *tpe_resources_resourcedescription_get(struct tpe *tpe, uint32_t resourceid);
uint64_t tpe_resources_resourcedescription_updated(struct tpe *tpe, uint32_t resource);

uint32_t tpe_resources_resourcedescription_get_by_name(struct tpe *,
		const char *name);
