
#ifndef USE_GUI
#define USE_GUI 1
#endif

struct tpe {
	int player; /* My player ID */
	char *playername;
	char *racename;
	int turn;

	struct tpe_event *event;
	struct servers *servers;
	struct tpe_orders *orders;
	struct tpe_obj *obj;
	struct tpe_board *board;
	struct tpe_resources *resources;
	struct tpe_ship *ship;
	struct tpe_sequence *sequence;


	struct gui *gui;

	struct ai *ai;
};


enum {
	TPE_ERR_GENERIC = -1,
	/* Using a TP04 message with TP03 servers */
	TPE_ERR_MSG_NOT_SUPPORTED = -7,
};
