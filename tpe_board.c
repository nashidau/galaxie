#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_board.h"
#include "tpe_event.h"
#include "tpe_msg.h"
#include "tpe_sequence.h"
#include "tpe_util.h"

struct tpe_board {
	FILE *msglog;
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

	int unread;
	int received;
};

/* Event handlers for messages */
static int tpe_board_msg_board_receive(void *data, int type, void *event);
static int tpe_board_msg_message_receive(void *data, int type, void *event);


static int tpe_board_board_changed_notify(struct tpe *tpe, struct board *board);

struct tpe_board *
tpe_board_init(struct tpe *tpe){
	struct tpe_board *board;
	FILE *msglog;

	if (tpe->board != NULL) return tpe->board;

	board = calloc(1,sizeof(struct tpe_board));

	board->boards = ecore_list_new();

	/* Events we send */
	tpe_event_type_add(tpe->event, "BoardUpdate");

	/* What messages we handle */
	tpe_event_handler_add(tpe->event, "MsgBoard",
                        tpe_board_msg_board_receive, tpe);
	tpe_event_handler_add(tpe->event, "MsgMessage",
                        tpe_board_msg_message_receive, tpe);

	/* Sequence system can handle... */
	tpe_sequence_register(tpe, "MsgGetBoardIDs",
				"MsgListOfBoards", 
				"MsgGetBoards",
				tpe_board_board_updated_get, NULL, NULL);

	/* FIXME: For now... we log messages */
	msglog = fopen("msglog.txt","w");
	board->msglog = msglog;

	return board;
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

uint64_t 
tpe_board_board_updated_get(struct tpe *tpe, uint32_t oid){
	struct board *board;

	board = tpe_board_board_get_by_id(tpe, oid);
	if (board)
		return board->updated;
	else
		return 0;
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
	struct message **nm;
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

	if (board->name) free(board->name);
	if (board->description) free(board->description);

	tpe_util_parse_packet(body, "issil", &id, &board->name, 
			&board->description, &board->nmessages,
			&board->updated);

	if (board->nmessages == board->nalloced)
		return 1;


	nm = realloc(board->messages, 
			sizeof(struct message) * board->nmessages);
	if (nm == NULL){
		/* XXX: Need an error reporting system */
		return 1; 
	}
	board->messages = nm;

	ntoget = board->nmessages - board->nalloced;
	toget = malloc(sizeof(int32_t) * (ntoget + 2));
	toget[0] = htonl(id);
	toget[1] = htonl(ntoget);

	for (i = board->nalloced ; i < board->nmessages ; i ++){
		board->messages[i] = NULL;
		toget[i - board->nalloced + 2] = htonl(i);
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
	message->unread = 1;

	tpe_util_parse_packet(body, "iiassir", &message->board,
			&message->slot, NULL, NULL,
			&message->title,
			&message->body,
			&message->turn,
			&message->nrefs, &message->references);

	fprintf(tpe->board->msglog,"Message is: Board %d Slot %d Turn %d\n"
		"  Subject: %s\n  Body: %s\n"
		"  Nrefs: %d\n",
			message->board,message->slot,message->turn,
			message->title, message->body,message->nrefs);

	board = tpe_board_board_get_by_id(tpe, message->board);
	if (board == NULL){
		printf("Weird - No board %d\n", message->board);
		free(message->title); 
		free(message->body);
		free(message);
		return 1;
	}

	if (board->messages[message->slot]){
		printf("Strange - I have this message Bord %d Slot %d\n",
			message->board, message->slot);
		free(message->title); 
		free(message->body);
		free(message);
		return 1;
	}

	board->messages[message->slot] = message;

	board->unread ++;
	board->received ++;

	tpe_board_board_changed_notify(tpe, board);

	return 1;
}

/**
 * Gets the first unread message on the specified board.
 *
 * Note this will happilly return NULL if there are no unread messages.
 *
 * @param tpe General tpe structure.
 * @param id The board to get message from.
 * @return Message structure, or NULL if there are no unread messages.
 */
struct message *
tpe_board_board_message_unread_get(struct tpe *tpe, uint32_t id){
	struct board *board;
	int i;

	if (tpe == NULL) return NULL;
	
	board = tpe_board_board_get_by_id(tpe, id);
	if (board == NULL) return NULL;

	for (i = 0 ; i < board->nmessages ; i ++){
		if (board->messages[i] && board->messages[i]->unread)
			return board->messages[i];
	}

	return NULL;
}

/**
 * Returns the first mesage in the most recent turn.
 *
 * This function returns the first message in highest turn.  This generally
 * the current turn, but is not necessarily.
 * 
 * Returns NULL if there are no messages in the board.
 * 
 * Note: This function is slow.
 *
 * @param tpe General tpe structure.
 * @param id Board to get message from.
 * @return Message 
 */
struct message *
tpe_board_board_message_turn_get(struct tpe *tpe, uint32_t id){
	struct message *message;
	struct board *board;
	int i;

	if (tpe == NULL) return NULL;
	
	board = tpe_board_board_get_by_id(tpe, id);
	if (board == NULL) return NULL;

	if (board->nmessages == 0) return NULL;
	if (board->messages[0] == NULL) return NULL;
	
	message = board->messages[0];

	for (i = 1 ; i < board->nmessages ; i ++){
		if (board->messages[i] == NULL) continue;
		if (board->messages[i]->turn > message->turn)
			message = board->messages[i];
	}

	return message;

}

/**
 * Gets the next message in the board.
 *
 * @param tpe Tpe structure
 * @param message A valid message
 * @return Next message, or the same message if last.  NULL on error.
 */
struct message *
tpe_board_board_message_next(struct tpe *tpe, struct message *msg){
	struct board *board;

	if (tpe == NULL || msg == NULL) return NULL;

	board = tpe_board_board_get_by_id(tpe, msg->board);
	assert(board);
	if (board == NULL) return NULL;

	if (board->nmessages > msg->slot + 1)
		return board->messages[msg->slot + 1];
	else 
		return msg;
}

/**
 * Gets the previous message in the board.
 *
 * @param tpe Tpe structure
 * @param message A valid message
 * @return Previous message, or the same message if last.  NULL on error.
 */

struct message *
tpe_board_board_message_prev(struct tpe *tpe, struct message *msg){
	struct board *board;

	if (tpe == NULL || msg == NULL) return NULL;

	board = tpe_board_board_get_by_id(tpe, msg->board);
	assert(board);
	if (board == NULL) return NULL;

	if (msg->slot > 0)
		return board->messages[msg->slot - 1];
	else 
		return msg;


}

/**
 * Get the first message in the next turn.
 *
 * Similar to tpe_board_board_message_next, except this will jump to the first
 * message in the next turn, OR the last message if there are no messages in
 * turn indicated.
 *
 *
 * @param tpe General TPE structure
 * @param msg The current message
 * @return Message as above.
 */
struct message *
tpe_board_board_message_next_turn(struct tpe *tpe, struct message *msg){
	struct board *board;
	int i;

	if (tpe == NULL || msg == NULL) return NULL;

	board = tpe_board_board_get_by_id(tpe, msg->board);
	assert(board);
	if (board == NULL) return NULL;

	for (i = msg->slot ; i < board->nmessages ; i ++){
		if (msg->turn != board->messages[i]->turn)
			return board->messages[i];
	}

	return board->messages[board->nmessages - 1];
}
struct message *
tpe_board_board_message_prev_turn(struct tpe *tpe, struct message *msg){
	struct board *board;
	int i;

	if (tpe == NULL || msg == NULL) return NULL;

	board = tpe_board_board_get_by_id(tpe, msg->board);
	assert(board);
	if (board == NULL) return NULL;
	for (i = msg->slot ; i > 0 ; i --){
		if (msg->turn != board->messages[i]->turn)
			return board->messages[i];
	}

	return board->messages[0];
}

/**
 * Marks a message as read.
 *
 * If the message is already read, it has no affect.  Otherwise it triggers a
 * board changed message.
 *
 * @param tpe TPE pointer
 * @param msg The message to mark as read
 * @return 0 on success, a negative failure code otherwise.
 */
int 
tpe_board_board_message_read(struct tpe *tpe, struct message *msg){
	struct board *board;

	if (tpe == NULL) return -1;
	if (msg == NULL) return -1;

	/* Nothing to do... */
	if (msg->unread == 0) return 0;
	
	msg->unread = 0;

	board = tpe_board_board_get_by_id(tpe, msg->board);
	/* Weird: Someone has screwed with msg */
	assert(board);
	if (board == NULL) return -1;

	board->unread --;

	tpe_board_board_changed_notify(tpe, board);

	return 0;
}

/** 
 * Sends a BoardUpdate message.
 *
 * Sends a board updated message with teh current status of hte board.
 *
 * @param tpe TPE structure.
 * @param board Board that has changed.
 * @return 0 on success, less then zero on error.
 */
static int
tpe_board_board_changed_notify(struct tpe *tpe, struct board *board){
	struct board_update *update;	

	assert(tpe);
	assert(board);

	update = calloc(1,sizeof(struct board_update));
	update->id = board->oid;
	update->name = board->name;
	update->desc = board->description;
	update->messages = board->received;
	update->unread = board->unread;

	tpe_event_send(tpe->event, "BoardUpdate", update, NULL, NULL);

	return 0;
}

