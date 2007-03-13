struct tpe_resources;
struct resourcedescription;

struct tpe_resources *tpe_resource_init(struct tpe *tpe);
struct resourcedescription *tpe_resource_resourcedescription_get(struct tpe *tpe, uint32_t resourceid);
uint64_t tpe_resource_resourcedescription_updated(struct tpe *tpe, uint32_t resource);
