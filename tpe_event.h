int tpe_event_init(void);
int tpe_event_type_add(const char *name);

int tpe_event_handler_add(const char *event,
		int (*handler)(void *data, int type, void *event), void *data);


int tpe_event_send(const char *name , void *event,
			void (*freefn)(void *data, void *event), void *data);

void tpe_event_nofree(void *,void *);
void tpe_event_free(void *,void *);


