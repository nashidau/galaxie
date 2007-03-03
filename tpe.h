


struct tpe {
	int player; /* My player ID */
	char *playername;
	char *racename;

	struct tpe_event *event;
	struct tpe_msg *msg;
	struct tpe_comm *comm;
	struct tpe_orders *orders;
	struct tpe_gui *gui;
	struct tpe_obj *obj;
	struct tpe_board *board;
	struct tpe_ship *ship;
	struct tpe_sequence *sequence;

	struct ai *ai;
};


