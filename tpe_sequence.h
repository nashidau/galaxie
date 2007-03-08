struct tpe_sequence;

struct tpe_sequence *tpe_sequence_init(struct tpe *tpe);
int tpe_sequence_register(struct tpe *tpe,
		const char *updatemsg,
		const char *oidlist,
		const char *getmsg,
		uint64_t (*lastupdatefn)(struct tpe *, uint32_t id),
		void (*list_begin)(struct tpe *),
		void (*list_end)(struct tpe *));
