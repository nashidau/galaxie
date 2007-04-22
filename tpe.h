


struct tpe {
	int player; /* My player ID */
	char *playername;
	char *racename;
	int turn;

	struct tpe_event *event;
	struct tpe_msg *msg;
	struct tpe_comm *comm;
	struct tpe_orders *orders;
	struct tpe_obj *obj;
	struct tpe_board *board;
	struct tpe_resources *resources;
	struct tpe_ship *ship;
	struct tpe_sequence *sequence;

	struct gui *gui;

	struct ai *ai;
};


