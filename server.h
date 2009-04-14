struct tpe;
struct servers;
struct server;

struct msg {
	struct server *server;
	struct tpe *tpe;
	const char *type;
	unsigned int seq;
	unsigned int len;
	unsigned int protocol;
	void *data;
	void *end;
};

typedef int (*msgcb)(void *userdata, struct msg *);
typedef int (*conncb)(void *userdata, struct server *mcon);

struct servers *server_init(struct tpe *tpe);

int server_send(struct server *msg, const char *type, 
		msgcb cb, void *userdata,
		void *data, int len);
int server_send_strings(struct server *msg, const char *type, 
		msgcb cb, void *userdata,
		...);
int server_send_format(struct server *msg, const char *type,
		msgcb cb, void *userdata,
		const char *format, ...);

struct server *
server_connect(struct tpe *tpe, const char *server, int port, int usessl,
		conncb cb, void *userdata);

int server_protocol_get(struct server *);

#ifndef ntohll
#define ntohll(x) (((int64_t)(ntohl((int)((x << 32) >> 32))) << 32) | 	\
                     (unsigned int)ntohl(((int)(x >> 32)))) //By Runner
#define htonll(x) ntohll(x)
#endif
