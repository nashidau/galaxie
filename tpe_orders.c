/* 
 * Very low level system for dealing with order types
 */
#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_event.h"
#include "tpe_msg.h"
#include "tpe_util.h"
#include "tpe_orders.h"

struct tpe_orders {
	struct tpe *tpe;

	Ecore_List *ordertypes;
};



struct order_desc {
	uint32_t otype;	 /* Type */
	const char *name;
	const char *description;
	
	int nargs;
	struct order_arg *args;

	uint64_t updated;
};

static int tpe_orders_msg_order_description_ids(void *, int type, void *data);
static int tpe_orders_msg_order_description(void *, int type, void *data);

struct tpe_orders *
tpe_orders_init(struct tpe *tpe){
	struct tpe_orders *orders;

	orders = calloc(1,sizeof(struct tpe_orders));
	orders->ordertypes = ecore_list_new();
	orders->tpe = tpe;

	tpe_event_handler_add(tpe->event, "MsgOrderDescription", /* 9 */
			tpe_orders_msg_order_description, tpe);
	tpe_event_handler_add(tpe->event, "MsgOrderDescriptionIDs", /* 33 */
			tpe_orders_msg_order_description_ids, tpe);

	/* Register some events?? */
	
	

	return orders;
}

int
tpe_order_get_type_by_name(struct tpe *tpe, const char *name){
	struct order_desc *od;
	ecore_list_goto_first(tpe->orders->ordertypes);
	while ((od = ecore_list_next(tpe->orders->ordertypes))){
		if (strcmp(name, od->name) == 0)
			return od->otype;
	}
	return -1;

}

static int
tpe_orders_msg_order_description_ids(void *data, int type, void *edata){
	struct tpe *tpe = data;
	int *event = edata;
	int noids;
	struct ObjectSeqID *oids = 0;
	int *toget;
	int i,n;
	int seqkey;
	int more;


	event += 4;

	tpe_util_parse_packet(event, "iiO", &seqkey, &more, &noids,&oids);

	toget = malloc(noids * sizeof(int) + 4);
	for (i  = 0 , n = 0; i < noids; i ++){
		toget[n + 1] = htonl(oids[i].oid);
		n ++;
	}
	toget[0] = htonl(n);
	
	tpe_msg_send(tpe->msg, "MsgGetOrderDescription" /* 8 */, NULL, NULL, 
			toget, n * 4 + 4);

	free(toget);

	return 1;
}

static int
tpe_orders_msg_order_description(void *data, int type, void *edata){
	struct tpe *tpe = data;
	int *event = edata;
	struct order_desc *od;

	od = calloc(1,sizeof(struct order_desc));

	printf("An order description\n");

	event += 4;

	tpe_util_parse_packet(event, "issQl",
			&od->otype,&od->name, &od->description, 
			&od->nargs, &od->args, &od->updated);


	printf("order %d is %s\n\t%s\n",od->otype, od->name,od->description);
	{ int i;
	for (i = 0 ; i < od->nargs ; i ++){
		printf("\tArg: %s  A type %d\n\t%s\n",od->args[i].name,
				od->args[i].arg_type, od->args[i].description);
	}
	}

	ecore_list_append(tpe->orders->ordertypes, od);

	return 1;
}
