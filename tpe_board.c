#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_board.h"
#include "tpe_event.h"
#include "tpe_msg.h"
#include "tpe_util.h"

struct tpe_board {
	Ecore_List *boards;
};

struct board {
	uint32_t oid;
	char *name;
	char *description;
	uint32_t nmessages;
	uint64_t updated;

	struct message **messages;
	int nalloced;
};

struct message {
	int board;
	int slot;
	char *title;
	char *body;
	int turn;
};

static int tpe_board_msg_board_list(void *data, int type, void *event);
static int tpe_board_msg_board_receive(void *data, int type, void *event);
static int tpe_board_msg_message_receive(void *data, int type, void *event);

struct tpe_board *
tpe_board_init(struct tpe *tpe){
	struct tpe_board *board;

	board = calloc(1,sizeof(struct tpe_board));

	board->boards = ecore_list_new();

	tpe_event_handler_add(tpe->event, "MsgListOfBoards", 
			tpe_board_msg_board_list, tpe);
	tpe_event_handler_add(tpe->event, "MsgBoard",
                        tpe_board_msg_board_receive, tpe);
	tpe_event_handler_add(tpe->event, "MsgMessage",
                        tpe_board_msg_message_receive, tpe);


	return board;
}


static int
tpe_board_msg_board_list(void *data, int type, void *event){
	struct tpe *tpe = data;
	int seqkey, more;
	int noids;
	struct ObjectSeqID *oids = NULL;
	struct board *board; 
	int *toget, i, n;

	/* FIXME: Should parse the header too */
	event = (char *)event + 16;		

	oids = NULL;
	tpe_util_parse_packet(event, "iiO", &seqkey, &more, &noids, &oids);

	toget = malloc(noids * sizeof(int) + 4);
	for (i = 0, n = 0 ; i < noids ; i ++){
		board = tpe_board_board_get_by_id(tpe, oids[i].oid);
		if (board == NULL || board->updated < oids[i].updated){
			toget[n + 1] = htonl(oids[i].oid);
			n ++;
		}
	}
          
	if (n){
		toget[0] = htonl(n);
		tpe_msg_send(tpe->msg, "MsgGetBoards",NULL, NULL,
				toget, n * 4 + 4);
	} 

	free(toget);
	free(oids);

	return 1;
}



struct board *
tpe_board_board_get_by_id(struct tpe *tpe, uint32_t oid){
        struct board *board;

        ecore_list_goto_first(tpe->board->boards);
        while ((board = ecore_list_next(tpe->board->boards))){
                if (board->oid == oid)
                        return board;
	}

        return NULL;
}

struct board *
tpe_board_board_add(struct tpe *tpe, uint32_t oid){
        struct board *board;
        board = calloc(1,sizeof(struct board));
        if (ecore_list_append(tpe->board->boards, board) == 0)
		printf("Error appending list\n");
	board->oid = oid;
        return board;
}


static int 
tpe_board_msg_board_receive(void *data, int type, void *event){
	struct tpe *tpe;
	char *body;
	struct board *board;
	int32_t id;
	int32_t *toget;
	int ntoget;
	int i;

	tpe = data;
	body = event;
	body += 16;

	tpe_util_parse_packet(body, "i", &id);

	board = tpe_board_board_get_by_id(tpe, id);
	if (board == NULL)
		board = tpe_board_board_add(tpe, id);

	tpe_util_parse_packet(body, "issil", &id, &board->name, 
			&board->description, &board->nmessages,
			&board->updated);

	if (board->nmessages == board->nalloced)
		return 1;


	/* FIXME: Realloc can fail (and leak) */
	board->messages = realloc(board->messages, 
			sizeof(struct message) * board->nmessages);

	ntoget = board->nmessages - board->nalloced;
	toget = malloc(sizeof(int32_t) * (ntoget + 2));
	toget[0] = htonl(id);
	toget[1] = htonl(board->nmessages);

	for (i = board->nalloced ; i < board->nmessages ; i ++){
		board->messages[i] = NULL;
		toget[i + 2] = htonl(i);
	}

	tpe_msg_send(tpe->msg, "MsgMessageGet", NULL, NULL, 
			toget, (ntoget + 2) * sizeof(uint32_t));

	board->nalloced = board->nmessages;

	free(toget);

	return 1;
}

/**
 * Callback handler for receiving a Message.
 *
 * Saves the new message in the boards list of messages.
 *
 * FIXME: Need to clean up at all exit paths 
 */
static int 
tpe_board_msg_message_receive(void *data, int type, void *event){
	struct board *board;
	struct message *message;
	struct tpe *tpe;
	char *body;

	tpe = data;
	body = event;
	body += 16;

	message = calloc(1,sizeof(struct message));

	/* FIXME: Doesn't handle reference system */
	tpe_util_parse_packet(body, "iiassi", &message->board,
			&message->slot, NULL, NULL,
			&message->title,
			&message->body,
			&message->turn);

	printf("Message is: Board %d Slot %d Turn %d\n"
		"  Subject: %s\n  Body: %s\n",
			message->board,message->slot,message->turn,
			message->title, message->body);

	board = tpe_board_board_get_by_id(tpe, message->board);
	if (board == NULL){
		printf("Weird - No board %d\n", message->board);
		return 1;
	}

	if (board->messages[message->slot]){
		printf("Strange - I have this message Bord %d Slot %d\n",
			message->board, message->slot);
		return 1;
	}

	board->messages[message->slot] = message;

	/* FIXME: Appropriate Notification */

	return 1;
}
