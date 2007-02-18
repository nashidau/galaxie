struct ObjectSeqID {
	int oid;
	int64_t updated;
};

char *tpe_util_string_extract(const char *src, int *lenp, const char **endp);
char *tpe_util_dump_packet(void *pdata);
int tpe_util_parse_packet(void *pdata, char *format, ...);

#define ntohll(x) (((int64_t)(ntohl((int)((x << 32) >> 32))) << 32) | 	\
                     (unsigned int)ntohl(((int)(x >> 32)))) //By Runner
#define htonll(x) ntohll(x)
