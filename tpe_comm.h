struct tpe;
struct tpe_comm;

struct tpe_comm *tpe_comm_init(struct tpe *);

int tpe_comm_connect(struct tpe_comm *, const char *, int port, const char *, const char *);

