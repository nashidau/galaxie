struct object;
struct ObjectSeqID {
	int oid;
	uint64_t updated;
};

struct reference {
	uint32_t type;
	uint32_t value;
};

char *tpe_util_string_extract(const char *src, int *lenp, const char **endp);
char *tpe_util_dump_packet(void *pdata);
int tpe_util_parse_packet(void *pdata, void *endp, char *format, ...);
uint64_t tpe_util_dist_calc2(struct object *obj1, struct object *obj2);

void * tpe_util_parse_array(void *buf, void *end, char *format, ...);

int tpe_util_test(void);

