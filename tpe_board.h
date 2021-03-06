
/**
 * Event object for board changing
 * 
 */
struct board_update {
	uint32_t id;
	const char *name;
	const char *desc;

	int unread;
	int messages;
};

struct message {
	int board;
	int slot;
	char *title;
	char *body;
	int turn;
	int unread;

	int nrefs;
	struct reference *references;

};


struct tpe_board *tpe_board_init(struct tpe *tpe);
struct board *tpe_board_board_get_by_id(struct tpe *tpe, uint32_t oid);
struct board *tpe_board_board_add(struct tpe *tpe, uint32_t oid);

uint64_t tpe_board_board_updated_get(struct tpe *tpe, uint32_t oid);

struct message *tpe_board_board_message_unread_get(struct tpe *, uint32_t);
struct message *tpe_board_board_message_turn_get(struct tpe *tpe, uint32_t id);
struct message *tpe_board_board_message_next(struct tpe *, struct message *);
struct message *tpe_board_board_message_prev(struct tpe *, struct message *);
struct message *tpe_board_board_message_next_turn(struct tpe*, struct message*);
struct message *tpe_board_board_message_prev_turn(struct tpe*, struct message*);
int tpe_board_board_message_read(struct tpe *tpe, struct message *msg);
