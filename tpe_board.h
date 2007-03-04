
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


struct tpe_board *tpe_board_init(struct tpe *tpe);
struct board *tpe_board_board_get_by_id(struct tpe *tpe, uint32_t oid);
struct board *tpe_board_board_add(struct tpe *tpe, uint32_t oid);

uint64_t tpe_board_board_updated_get(struct tpe *tpe, uint32_t oid);
