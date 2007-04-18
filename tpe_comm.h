struct tpe;
struct tpe_comm;

enum connectstatus {
	CONSTATUS_NONE, 
	CONSTATUS_CONNECTING,
	CONSTATUS_PROTOCOLNEGOTIATION,
	CONSTATUS_FILTERNEGOTIATION,
	CONSTATUS_SSLNEGOTIATION,
	CONSTATUS_FEATURENEGOTIATION,
	CONSTATUS_CONNECTED,
};	

struct connect {
	const char *server;
	const char *user;
	const char *game;
	enum connectstatus status;
};

struct tpe_comm *tpe_comm_init(struct tpe *);

int tpe_comm_connect(struct tpe_comm *, const char *, int port, 
		const char *game,
		const char *, const char *);

