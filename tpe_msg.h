struct tpe;
struct tpe_msg;
struct tpe_msg_connection;

typedef int (*msgcb)(void *userdata, const char *msgtype, int len, void *data);
typedef int (*conncb)(void *userdata, struct tpe_msg_connection *mcon);

struct tpe_msg *tpe_msg_init(struct tpe *tpe);
int tpe_msg_send(struct tpe_msg *msg, const char *type, 
		msgcb cb, void *userdata,
		void *data, int len);
int tpe_msg_send_strings(struct tpe_msg *msg, const char *type, 
		msgcb cb, void *userdata,
		...);
int tpe_msg_send_format(struct tpe_msg *msg, const char *type,
		msgcb cb, void *userdata,
		const char *format, ...);

int tpe_msg_connect(struct tpe_msg *msg, 
		const char *server, int port, int usessl,
		conncb cb, void *userdata);

#ifndef ntohll
#define ntohll(x) (((int64_t)(ntohl((int)((x << 32) >> 32))) << 32) | 	\
                     (unsigned int)ntohl(((int)(x >> 32)))) //By Runner
#define htonll(x) ntohll(x)
#endif
