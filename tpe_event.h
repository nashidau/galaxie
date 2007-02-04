struct tpe_event;
struct tpe;

struct tpe_event *tpe_event_init(struct tpe *tpe);
int tpe_event_type_add(struct tpe_event *tpeev, const char *name);

int tpe_event_handler_add(struct tpe_event *tpeev, const char *event,
		int (*handler)(void *data, int type, void *event), void *data);


int tpe_event_send(struct tpe_event *tpe_ev, const char *name , void *event,
			void (*freefn)(void *data, void *event), void *data);


