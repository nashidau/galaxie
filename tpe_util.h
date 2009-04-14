#include <inttypes.h>
#include <stddef.h> /* ptrdiff_t */

struct object;
struct ObjectSeqID {
	int oid;
	uint64_t updated;
};

struct reference {
	uint32_t type;
	uint32_t value;
};

enum parsetype {
	PARSETYPE_END = -1,
	PARSETYPE_INT,
	PARSETYPE_LONG,
	PARSETYPE_LIST,
	PARSETYPE_STRING,
	PARSETYPE_STRUCT,
	PARSETYPE_ARRAYOF = 0x1000,
	PARSETYPE_COMPLEX = PARSETYPE_ARRAYOF | PARSETYPE_STRUCT,
};

struct parseitem {
	enum parsetype type;
	ptrdiff_t off;
	intptr_t lenoff;
	struct parseitem *sub;
	const char *subtype;
	size_t subsize;
};

 

char *tpe_util_string_extract(const char *src, int *lenp, const char **endp);
char *tpe_util_dump_packet(void *pdata);
int tpe_util_parse_packet(void *pdata, void *endp, char *format, ...);
uint64_t tpe_util_dist_calc2(struct object *obj1, struct object *obj2);

void * tpe_util_parse_array(void *buf, void *end, size_t size, char *format, ...);

void *parse_block(const char *buf,
		struct parseitem *items, 
		void *data, /* Where to put it of NULL */
		const char *type,
		size_t size,
		char **end);	

int tpe_util_test(void);

