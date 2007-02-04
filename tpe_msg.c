/**
 * Messaging for the system
 * Controls the connection to the server, and the messages and interactions
 * going on.
 *
 * Decodes and encodes frames
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore.h>
#include <Ecore_Con.h>

#include "tpe_msg.h"
#include "tpe.h"
#include "tpe_event.h"

/*
 * For reference:
 *    struct _Ecore_Con_Event_Server_Data
     {
        Ecore_Con_Server *server;
        void             *data;
        int               size;
     };
 */

enum {
	HEADER_SIZE = 16
};

#define ID "Thousand Parsec - Enlightened Client"


struct msgname {
	const char *name;
	int tp03;
} msgnames[] = {
	{ "MsgOK", 			 0 },
	{ "MsgFail", 			 1 },
	{ "MsgSEQUENCE", 		 2 },
	{ "MsgConnect", 		 3 },
	{ "MsgLogin", 			 4 },	
	{ "MsgGET_OBJECTS_BY_ID", 	 5 },
	{ "MsgUNUSED1", 		 6 },
	{ "MsgOBJECT", 			 7 },
	{ "MsgGET_ORDER_DESCRIPTION",    8 },
	{ "MsgORDER_DESCRIPTION", 	 9 },
	{ "MsgGET_ORDER", 		10 },
	{ "MsgORDER", 			11 },
	{ "MsgINSERT_ORDER", 		12 },
	{ "MsgREMOVE_ORDER", 		13 },
	{ "MsgGET_TIME_REMAINING", 	14 },
	{ "MsgTIME_REMAINING", 		15 },
	{ "MsgGET_BOARDS", 		16 },
	{ "MsgBOARD", 			17 },
	{ "MsgGET_MESSAGE", 		18 },
	{ "MsgMESSAGE", 		19 },
	{ "MsgPOST_MESSAGE", 		20 },
	{ "MsgREMOVE_MESSAGE", 		21 },
	{ "MsgGET_RESOURCE_DESCRIPTION",22 },
	{ "MsgRESOURCE_DESCRIPTION", 	23 },
	{ "MsgREDIRECT", 		24 },
	{ "MsgGET_FEATURES", 		25 },
	{ "MsgAVAILABLE_FEATURES", 	26 },
	{ "MsgPING", 			27 },
	{ "MsgGET_OBJECT_IDS", 		28 },
	{ "MsgGET_OBJECT_IDS_BY_POSITION", 	29 },
	{ "MsgGET_OBJECT_IDS_BY_CONTAINER", 	30 },
	{ "MsgLIST_OF_OBJECT_IDS", 		31 },
	{ "MsgGET_ORDER_DESCRIPTION_IDS", 	32 },
	{ "MsgLIST_OF_ORDER_DESCRIPTION_IDS", 	33 },
	{ "MsgPROBE_ORDER", 			34 },
	{ "MsgGET_BOARD_IDS", 			35 },
	{ "MsgLIST_OF_BOARDS", 			36 },
	{ "MsgGET_RESOURCES_IDS", 		37 },
	{ "MsgLIST_OF_RESOURCES_IDS", 		38 },
	{ "MsgGET_PLAYER_DATA", 		39 },
	{ "MsgPLAYER_DATA", 			40 },
	{ "MsgGET_CATEGORY", 			41 },
	{ "MsgCATEGORY", 			42 },
	{ "MsgADD_CATEGORY", 			43 },
	{ "MsgREMOVE_CATEGORY", 		44 },
	{ "MsgGET_CATEGORY_IDS", 		45 },
	{ "MsgLIST_OF_CATEGORY_IDS", 		46 },
	{ "MsgGET_DESIGN", 			47 },
	{ "MsgDESIGN", 				48 },
	{ "MsgADD_DESIGN", 			49 },
	{ "MsgMODIFY_DESIGN", 			50 },
	{ "MsgREMOVE_DESIGN", 			51 },
	{ "MsgGET_DESIGN_IDS", 			52 },
	{ "MsgLIST_OF_DESIGN_IDS", 		53 },
	{ "MsgGET_COMPONENT", 			54 },
	{ "MsgCOMPONENT", 			55 },
	{ "MsgGET_COMPONENT_IDS", 		56 },
	{ "MsgLIST_OF_COMPONENT_IDS", 		57 },
	{ "MsgGET_PROPERTY", 			58 },
	{ "MsgPROPPERY", 			59 },
	{ "MsgGET_PROPERTY_IDS", 		60 },
	{ "MsgLIST_OF_PROPERTY_IDS", 		61 },
	{ "MsgACCOUNT_CREATE", 			62 },
};




/**
 * Status structure for tpe_msg.  Keeps track of all the important info
 * for the system.
 */
struct tpe_msg {
	/* Actual server connection */
	Ecore_Con_Server *svr;

	/* Sequence */
	unsigned int seq;
	struct tpe_msg_cb *cbs;

	/* Header */
	unsigned int header;

	/* Signals */
	int *signals;


	/* Buffered Data */
	struct {
		int size;
		char *data;
	} buf;


};



struct tpe_msg_cb {
	struct tpe_msg_cb *next,*prev;

	int seq;

	msgcb 	cb;
	void *userdata;

	int deleteme;

};


static void tpe_msg_event_register(struct tpe *tpe);


static int tpe_msg_receive(void *data, int type, void *edata);
//static int tpe_msg_connect(void *data, int type, void *edata);
static int tpe_msg_cb_add(struct tpe_msg *msg, int seq, msgcb cb, void *userdata);
//static void tpe_connect_logged_in(void *msg, const char *type, int len, void *mdata);
//static int tpe_msg_register_events(struct tpe_msg *msg);
//static void tpe_connect_accept(void *msg, const char * type, int len,
//		void *mdata);
static void tpe_msg_handle_packet(struct tpe_msg *msg, int seq, int type, 
			int len, void *data);


struct tpe_msg *
tpe_msg_init(struct tpe *tpe){
	struct tpe_msg *msg;

	ecore_con_init();

	msg = calloc(1,sizeof(struct tpe_msg));
	tpe->msg = msg;
	if (msg == NULL) return NULL;

	
	msg->seq = 1;
	msg->header = ('T' << 0) | ('P' << 8) | ('0' << 16) | ('3' << 24);
	
	/* Register events */
	tpe_msg_event_register(tpe);

//	ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, 	
//			tpe_msg_connect, msg);
	ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA,
			tpe_msg_receive, msg);
	ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA,
			tpe_msg_receive, msg);
	

	return msg;
}

static void
tpe_msg_event_register(struct tpe *tpe){
	int i,n;

	n = sizeof(msgnames)/sizeof(msgnames[0]);

	for (i = 0 ; i < n ; i ++){
		tpe_event_type_add(tpe->event, msgnames[i].name);
	}
}

/***************************************/

/* 
 * Callback to say Ecore_Con has connected
 *
 * Now we will attempt to connect
 */
static int tpe_msg_connect(void *data, int type, void *edata){
	//struct tpe_msg *msg; 
	printf("Connected\n");

	//msg = data;
	//tpe_msg_send_strings(msg, TPE_MSG_CONNECT, tpe_connect_accept,msg,ID, NULL);

	return 0;
}

/* FIXME: These need to be moved elsewher */
#if 0
static void 
tpe_connect_accept(void *msg, enum tpe_msg_type type, int len,
		void *mdata){
	tpe_msg_send_strings(msg, TPE_MSG_LOGIN,  tpe_connect_logged_in, msg,
			"nash", "pass", 0);
	tpe_msg_send(msg, TPE_MSG_GET_FEATURES, 0,0,0,0);
}
#endif 
#if 0
static void
tpe_connect_logged_in(void *msg, enum tpe_msg_type type, int len, void *mdata){
	int buf[3];
	printf("Logged in - I hope\n");

	tpe_msg_send(msg, TPE_MSG_GET_TIME_REMAINING, 0, 0,0,0);
	
	/* Fire of some sequences */
	buf[0] = htonl(-1);	/* New seq */
	buf[1] = htonl(0);	/* From 0 */
	buf[2] = htonl(-1);	/* Get them all */

	tpe_msg_send(msg, TPE_MSG_GET_BOARD_IDS, 0,0,buf,12);
	tpe_msg_send(msg, TPE_MSG_GET_RESOURCES_IDS, 0,0,buf,12);
	tpe_msg_send(msg, TPE_MSG_GET_OBJECT_IDS, 0,0,buf,12);
}
#endif


static int 
tpe_msg_receive(void *udata, int ecore_event_type, void *edata){
	struct tpe_msg *msg;
	Ecore_Con_Event_Server_Data *data;
	unsigned int *header;
	char *start;
	unsigned int len, type, seq, remaining;

	msg = udata;
	data = edata;

	if (msg->buf.size){
		start = realloc(msg->buf.data, msg->buf.size + data->size);
		remaining = msg->buf.size + data->size;
		memcpy(start + data->size, data->data, data->size);
		msg->buf.data = start; /* Save it to free later */
	} else {
		start = data->data;
		remaining = data->size;
	}

	while (remaining > 16){
		header = (uint32_t *)start;
		seq = ntohl(header[1]);
		type = ntohl(header[2]);
		len = ntohl(header[3]);
		if (len + 16 > remaining)
			break;
		tpe_msg_handle_packet(msg, seq, type, len, start);
		start += len + 16;
		remaining -= len + 16;
	}

	if (remaining)
		printf("Untested: %d bytes remain\n", remaining);
	msg->buf.data = realloc(msg->buf.data, remaining);
	msg->buf.size = remaining;

	return 1;
}

static void
tpe_msg_handle_packet(struct tpe_msg *msg, int seq, int type, 
			int len, void *data){
	struct tpe_msg_cb *cb,*next;

	if (seq){
		for (cb = msg->cbs ; cb ; cb = next){
			next = cb->next;
			if (cb->seq == seq){
				if (cb->cb)
					cb->cb(cb->userdata, type, len, 
							(char*)data + 16);
				cb->deleteme = 1;
			} 
		}

		/* Now clean things up without callbacks */
		for (cb = msg->cbs ; cb ; cb = next){
			next = cb->next;
			if (cb->deleteme){
				if (cb->prev)
					cb->prev->next = cb->next;
				else
					msg->cbs = cb->next;
				if (cb->next)
					cb->next->prev = cb->prev;
				free(cb);
			} 
		}

	}


	{
		/* FIXME: Neeed to process this a little */
		int *edata;
		printf("Handling a %d\n",type);
		edata = malloc(16 + len);
		memcpy(edata,data,16+len);
		ecore_event_add(msg->signals[type], edata, NULL, NULL);
	}

}	

int
tpe_msg_send(struct tpe_msg *msg, const char *msgtype, 
		msgcb cb, void *userdata,
		void *data, int len){
	unsigned int *buf;	
	int type = -1;
	int i,n;

	if (msg == NULL) exit(1);
	if (len > 100000) exit(1);
	if (data == NULL && len != 0) exit(1);

	/* FIXME: Better then linear would be nice .. */
	n = sizeof(msgnames)/sizeof(msgnames[0]);
	for (i = 0 ; i < n ; i ++){
		if (strcmp(msgtype,msgnames[i].name) == 0)
			type = msgnames[i].tp03;
	}

	if (type == -1){
		printf("Message type %s not found\n",msgtype);
		exit(1);
	}

	buf = malloc(len + HEADER_SIZE);
	if (buf == NULL) exit(1);

	msg->seq ++;

	buf[0] = msg->header;
	buf[1] = htonl(msg->seq);
	buf[2] = htonl(type);
	buf[3] = htonl(len);
	memcpy(buf + 4, data, len);
printf("Sending %d/%d/%d [%p]\n",msg->seq,type,len,cb);
	ecore_con_server_send(msg->svr, buf, len + HEADER_SIZE);

	return tpe_msg_cb_add(msg, msg->seq, cb, userdata);
}



int
tpe_msg_send_strings(struct tpe_msg *msg, const char *msgtype,
		msgcb cb, void *userdata,
		...){
	va_list ap;
	int total,len,lens,pos,rv;
	const char *str;
	int nstrs = 0;
	char *buf;

	for (va_start(ap,userdata), total = 0 ; 
			(str = va_arg(ap,const char *)); ){
		total += strlen(str);
		nstrs ++;
	}
	va_end(ap);

	if (total == 0) return tpe_msg_send(msg, msgtype, cb, userdata, NULL, 0);
	
	total += nstrs * 4;
	buf = malloc(total);


	pos = 0;
	for (va_start(ap,userdata), len = 0 ; 
			(str = va_arg(ap,const char *)); ){
		len =strlen(str);
		lens = htonl(len);
		memcpy(buf + pos, &lens, 4);
		pos += 4;

		memcpy(buf + pos, str, len);
		pos += len;
	}
	va_end(ap);

	rv = tpe_msg_send(msg, msgtype, cb, userdata, buf, total);
	free(buf);
	return rv;
}

static int
tpe_msg_cb_add(struct tpe_msg *msg, int seq, msgcb cb, void *userdata){
	struct tpe_msg_cb *tmcb,*tmp;

	tmcb = calloc(1,sizeof(struct tpe_msg_cb));
	tmcb->seq = seq;
	tmcb->cb = cb;
	tmcb->userdata = userdata;
	tmcb->next = NULL;
	tmcb->prev = NULL;
	
	if (msg->cbs == NULL)
		msg->cbs = tmcb;
	else {
		for (tmp = msg->cbs ; tmp->next ; tmp = tmp->next)
			;
		tmp->next = tmcb;
		tmcb->prev = tmp;
	}

	return 0;
}




