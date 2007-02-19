


struct tpe_board {
	int dummy;
};

struct tpe_board *
tpe_board_init(struct tpe *tpe){
	struct tpe_board *board;

	board = calloc(1,sizeof(struct board));

	tpe_event_handler_add(tpe->event, "MsgListOfBoards", 
			tpe_board_msg_board_list, tpe);

	return board;
}


static int
tpe_board_msg_board_list(void *data, int type, void *event){
	struct tpe *tpe = data;

	/* FIXME: Should parse this too */
	event = (char *)event + 16;		

	tpe_util_parse_packet(event, "iiO", );

}
