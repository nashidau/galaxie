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
	struct server *server;
	const char *servername;
	const char *user;
	const char *game;
	enum connectstatus status;
	const char *pass;
	unsigned int triedcreate : 1;
	unsigned int accountregister : 1;
};

void tpe_comm_init(struct tpe *);

int tpe_comm_connect(struct tpe*, const char *server, int port, 
		const char *game,
		const char *user, const char *pass);

