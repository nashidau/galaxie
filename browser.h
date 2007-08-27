struct browser;
struct tpe;
struct server;

int browser_add(struct tpe *tpe, const char *server);

#define SERVER_MSG_VALID(msg) 					\
	do {							\
		assert(msg != NULL);				\
		assert(msg->server != NULL);			\
		assert(msg->type != NULL);			\
		assert(msg->len == 0 || msg->data != NULL);	\
		assert((char *)msg->data + msg->len == msg->end);\
	} while (0)

