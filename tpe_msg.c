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
	{ "MsgGetObjectsByID",		 5 },
	{ "MsgUNUSED1", 		 6 },
	{ "MsgObject", 			 7 },
	{ "MsgGetOrderDescription",	 8 },
	{ "MsgOrderDescription",	 9 },
	{ "MsgGetOrder", 		10 },
	{ "MsgOrder", 			11 },
	{ "MsgInsertOrder", 		12 },
	{ "MsgREMOVE_ORDER", 		13 },
	{ "MsgGetTimeRemaining", 	14 },
	{ "MsgTimeRemaining", 		15 },
	{ "MsgGetBoards", 		16 },
	{ "MsgBoard", 			17 },
	{ "MsgMessageGet", 		18 },
	{ "MsgMessage", 		19 },
	{ "MsgPOST_MESSAGE", 		20 },
	{ "MsgREMOVE_MESSAGE", 		21 },
	{ "MsgGET_RESOURCE_DESCRIPTION",	22 },
	{ "MsgRESOURCE_DESCRIPTION", 		23 },
	{ "MsgREDIRECT", 			24 },
	{ "MsgGetFeatures", 			25 },
	{ "MsgAvailableFeatures", 		26 },
	{ "MsgPING", 				27 },
	{ "MsgGetObjectIDs", 			28 },
	{ "MsgGET_OBJECT_IDS_BY_POSITION", 	29 },
	{ "MsgGET_OBJECT_IDS_BY_CONTAINER", 	30 },
	{ "MsgListOfObjectIDs", 		31 },
	{ "MsgGetOrderDescriptionIDs",	 	32 },
	{ "MsgOrderDescriptionIDs", 		33 },
	{ "MsgProbeOrder", 			34 },
	{ "MsgGetBoardIDs", 			35 },
	{ "MsgListOfBoards", 			36 },
	{ "MsgGetResourceIDs",	 		37 },
	{ "MsgLIST_OF_RESOURCES_IDS", 		38 },
	{ "MsgGetPlayerData",	 		39 },
	{ "MsgPlayerData", 			40 },
	{ "MsgGET_CATEGORY", 			41 },
	{ "MsgCATEGORY", 			42 },
	{ "MsgADD_CATEGORY", 			43 },
	{ "MsgREMOVE_CATEGORY", 		44 },
	{ "MsgGET_CATEGORY_IDS", 		45 },
	{ "MsgLIST_OF_CATEGORY_IDS", 		46 },
	{ "MsgGetDesign", 			47 },
	{ "MsgDesign", 				48 },
	{ "MsgADD_DESIGN", 			49 },
	{ "MsgMODIFY_DESIGN", 			50 },
	{ "MsgREMOVE_DESIGN", 			51 },
	{ "MsgGetDesignIDs", 			52 },
	{ "MsgListOfDesignIDs", 		53 },
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
	struct tpe *tpe;

	/* Actual server connection */
	Ecore_Con_Server *svr;

	/* Callbacks for connect */
	conncb conncb;
	void *conndata;

	/* Sequence */
	unsigned int seq;
	struct tpe_msg_cb *cbs;

	/* Header */
	unsigned int header;

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
static int tpe_msg_con_event_server_add(void *data, int type, void *edata);
static int tpe_msg_cb_add(struct tpe_msg *msg, int seq, msgcb cb, void *userdata);
//static void tpe_connect_logged_in(void *msg, const char *type, int len, void *mdata);
//static int tpe_msg_register_events(struct tpe_msg *msg);
//static void tpe_connect_accept(void *msg, const char * type, int len,
//		void *mdata);
static void tpe_msg_handle_packet(struct tpe_msg *msg, int seq, int type, 
			int len, void *data);

static int format_msg(int32_t *buf, const char *format, va_list ap);

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
	msg->tpe = tpe;

	ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, 	
			tpe_msg_con_event_server_add, msg);
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

/**
 * Initialise a connection to a server
 */
int tpe_msg_connect(struct tpe_msg *msg, 
		const char *server, int port, int usessl,
		conncb cb, void *userdata){
	Ecore_Con_Server *svr;

	/* FIXME: ssl */
	svr = ecore_con_server_connect(ECORE_CON_REMOTE_SYSTEM,
				server,port,msg);

	msg->svr = svr;

	msg->conncb = cb;
	msg->conndata = userdata;

	return 0;
}

/***************************************/

/* 
 * Callback to say Ecore_Con has connected
 *
 * Now we will attempt to connect
 */
static int tpe_msg_con_event_server_add(void *data, int type, void *edata){
	struct tpe_msg *msg;

	msg = data;

	if (msg->conncb)
		msg->conncb(msg->conndata, NULL);
	
	msg->conncb = 0;
	msg->conndata = 0;

	//msg = data;
	//tpe_msg_send_strings(msg, TPE_MSG_CONNECT, tpe_connect_accept,msg,ID, NULL);

	return 0;
}

static int 
tpe_msg_receive(void *udata, int ecore_event_type, void *edata){
	struct tpe_msg *msg;
	Ecore_Con_Event_Server_Data *data;
	unsigned int *header;
	char *start;
	unsigned int len, type, seq, remaining;
	int magic;

	msg = udata;
	data = edata;

	if (msg->buf.size){
		start = realloc(msg->buf.data, msg->buf.size + data->size);
		remaining = msg->buf.size + data->size;
		memcpy(start + msg->buf.size, data->data, data->size);
		msg->buf.data = start; /* Save it to free later */
	} else {
		start = data->data;
		remaining = data->size;
		msg->buf.data = NULL; /* Just to check */
	}

	while (remaining > 16){
		header = (uint32_t *)start;
		magic = header[0];
		if (strncmp("TP03", (char *)&magic, 4) != 0){
			printf("Invalid magic ;%.4s;\n",(char *)&magic);
			exit(1);
		}
		seq = ntohl(header[1]);
		type = ntohl(header[2]);
		len = ntohl(header[3]);
		if (len + 16 > remaining)
			break;
		tpe_msg_handle_packet(msg, seq, type, len, start);
		start += len + 16;
		remaining -= len + 16;
	}

	if (remaining){
		/* Malloc a new buffer to save it in */
		char *tmp;
		tmp = malloc(remaining);
		memcpy(tmp, start, remaining);
		free(msg->buf.data);

		msg->buf.data = tmp;
		msg->buf.size = remaining;
	} else {
		free(msg->buf.data);
		msg->buf.data = NULL;
		msg->buf.size = 0;
	}
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
					cb->cb(cb->userdata, "FIXME", len, 
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
		int *edata,i;
		const char *event = "None!";
		for (i = 0 ; i < 100 ; i ++){ /* FIXME: Not 100 */
			if (msgnames[i].tp03 == type){
				event = msgnames[i].name;
				break;
			}
		}
		printf("Handling a %s (%d) (response: %d)\n",event,type,seq);

		edata = malloc(16 + len);
		memcpy(edata,data,16+len);

		/* Default cleanup will free buffer */
		tpe_event_send(msg->tpe->event, event, edata, tpe_event_free, NULL);
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
printf("Sending Seq %d Type %d Len: %d [%p]\n",msg->seq,type,len,cb);
	ecore_con_server_send(msg->svr, buf, len + HEADER_SIZE);

	free(buf);

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

/* 
 *
 * Format options:
 *  i : Integer (one arg: int32_t)
 *  l : Long integer (one arg: uint64_t)
 *  0 : A zero (no arg)
 *  s : String (one arg: char *)
 */
int 
tpe_msg_send_format(struct tpe_msg *msg, const char *type,
		msgcb cb, void *userdata,
		const char *format, ...){
	va_list ap;
	int32_t *buf;
	int len;

	buf = NULL;
	va_start(ap, format);
	len = format_msg(buf, format, ap);
	va_end(ap);

	if (len > 0){
		buf = malloc(sizeof(int32_t) * len);
		va_start(ap, format);
		format_msg(buf, format, ap);
		va_end(ap);
	}

	return tpe_msg_send(msg, type, cb, userdata, buf, len*sizeof(int32_t));
}

static int
format_msg(int32_t *buf, const char *format, va_list ap){
	int32_t val;
	int64_t val64;
	char *str;
	int pos,extra;

	pos = 0;

	while (*format){
		switch (*format){
		case '0':
			if (buf)
				buf[pos] = 0;
			pos ++;
			break;
		case 'i':
			val = va_arg(ap, int32_t);
			if (buf)
				buf[pos] = htonl(val);
			pos ++;
			break;
		case 'l':
			val64 = va_arg(ap, int64_t);
			val64 = htonll(val64);
			if (buf)
				memcpy(buf + pos,&val64,sizeof(int64_t));
			pos += 2;
			break;
		case 's':
			str = va_arg(ap, char *);
			if (str)
				val = strlen(str);
			else
				val = 0;
			/* We pad with '\0' */
			if (val % 4)
				extra = 4 - val % 4;
			else
				extra = 0;
			if (buf)
				buf[pos] = htonl(val + extra);
			pos ++;
			if (buf)
				strncpy((char*)(buf + pos),str,val+extra);
			pos += (val + extra) % 4;
			break;
			
		}
		format ++;
	}

	return pos;
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




