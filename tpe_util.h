struct object;
struct ObjectSeqID {
	int oid;
	uint64_t updated;
};

char *tpe_util_string_extract(const char *src, int *lenp, const char **endp);
char *tpe_util_dump_packet(void *pdata);
int tpe_util_parse_packet(void *pdata, char *format, ...);
uint64_t tpe_util_dist_calc2(struct object *obj1, struct object *obj2);


