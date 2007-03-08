#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_event.h"
#include "tpe_sequence.h"
#include "tpe_msg.h"
#include "tpe_util.h"

struct sequence {
	struct tpe *tpe;
	const char *updatemsg;
	const char *getmsg;
	uint64_t (*lastupdatefn)(struct tpe *, uint32_t id);
	void (*list_begin)(struct tpe *);
	void (*list_end)(struct tpe *);
};

struct tpe_sequence {
	Ecore_List *seqs;
};

static int tpe_sequence_new_turn(void *data, int eventid, void *event);
static int tpe_sequence_handle_oids(void *udata, int type, void *event);

struct tpe_sequence *
tpe_sequence_init(struct tpe *tpe){
	struct tpe_sequence *tpeseq;
	if (tpe->sequence) return tpe->sequence;

	tpeseq = calloc(1,sizeof(struct tpe_sequence));
	tpeseq->seqs = ecore_list_new();

	tpe_event_handler_add(tpe->event,"NewTurn", tpe_sequence_new_turn, tpe);

	return tpeseq;
}


/**
 * A registered sequence will automatically handle sequence updates
 *
 *
 */
int
tpe_sequence_register(struct tpe *tpe,
		const char *updatemsg,
		const char *oidlist,
		const char *getmsg,
		uint64_t (*lastupdatefn)(struct tpe *, uint32_t id),
		void (*list_begin)(struct tpe *),
		void (*list_end)(struct tpe *)){
	struct sequence *seq;

	if (tpe == NULL) return -1;
	if (tpe->sequence == NULL){
		tpe->sequence = tpe_sequence_init(tpe);
	}

	seq = calloc(1,sizeof(struct sequence));
	seq->updatemsg = strdup(updatemsg);
	seq->getmsg = strdup(getmsg);
	seq->lastupdatefn = lastupdatefn;
	seq->list_begin = list_begin;
	seq->list_end = list_end;
	seq->tpe = tpe;

	ecore_list_append(tpe->sequence->seqs, seq);

	tpe_event_handler_add(tpe->event, oidlist, tpe_sequence_handle_oids, seq);

	return 0; 
}

/** 
 * New turn handler...
 *  - Fire of list of update requests for each sequence 
 */

static int 
tpe_sequence_new_turn(void *data, int eventid, void *event){
	struct tpe *tpe;
	struct sequence *seq;
	Ecore_List *seqs;

	tpe = data;

	printf("Sequence new turn handler\n");

	seqs = tpe->sequence->seqs;
	
	seq = ecore_list_goto_first(seqs);
	while ((seq = ecore_list_next(seqs))){
		if (seq->list_begin)
			seq->list_begin(tpe);
		tpe_msg_send_format(tpe->msg, seq->updatemsg,
				NULL, NULL,
				"i0i", -1,-1);
	}

	return 1;
}

/**
 * List of OIDS back...
 * 	- Check response type is what I expect
 * 	- use tpe_util to parse the list
 * 	- Is there more?
 * 		- Send of request for more seqence
 * 	- For each item
 * 		- Get last updated time for this item
 * 		- is it changed?
 * 			- Add to list of things to get
 * 	- If there are items to get
 * 		- Request them
 *
 * FIXME: Check lengths correctly...
 */
static int
tpe_sequence_handle_oids(void *udata, int type, void *event){
	struct sequence *seq;
	uint32_t *toget;
	int i,n;
	int seqkey, more;
	int noids;
	struct ObjectSeqID *oids = NULL;
	int64_t updated;

	seq = udata;

	tpe_util_parse_packet(event, "iiiiiiO", NULL, NULL, NULL, NULL,
			&seqkey, &more, &noids,&oids);

	toget = malloc((noids + 1) * sizeof(int));
	for (i = 0 , n = 0; i < noids ; i ++){
		updated = seq->lastupdatefn(seq->tpe, oids[i].oid);
		if (updated < oids[i].updated)
			toget[++n] = htonl(oids[i].oid);
	}

	if (n > 0){
		toget[0] = htonl(n);
		tpe_msg_send(seq->tpe->msg, seq->getmsg, NULL, NULL,
				toget, (n + 1) * sizeof(uint32_t));

	}

	free(toget);
	free(oids);

	if (more > 0){
		printf("FIXME: Need to handle more on sequences\n");
		printf("MORE MORE MORE MORE\n");
	} else {
		if (seq->list_end)
			seq->list_end(seq->tpe);
	}

	return 1;
}
		


