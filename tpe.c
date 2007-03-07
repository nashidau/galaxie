#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Con.h>
#include <Edje.h>

#include "tpe.h"
#include "tpe_board.h"
#include "tpe_comm.h"
#include "tpe_event.h"
#include "tpe_gui.h"
#include "tpe_msg.h"
#include "tpe_obj.h"
#include "tpe_orders.h"
#include "tpe_sequence.h"
#include "tpe_ship.h"

#include "ai_smith.h"


static void parse_args(int argc, char **argv);
int parse_username(int i, char **args);

struct args {
	const char *arg;
	int (*fn)(int i, char **args);
} args[] = {
	{ "--user",	parse_username },
	{ "-u",		parse_username },
};


int
main(int argc, char **argv){
	struct tpe *tpe;

	evas_init();
	ecore_init();

	tpe = calloc(1,sizeof(struct tpe));
	if (tpe == NULL) exit(1);

	parse_args(argc, argv);

	tpe->event 	= tpe_event_init(tpe);
	tpe->msg   	= tpe_msg_init(tpe);
	tpe->comm  	= tpe_comm_init(tpe);
	tpe->sequence 	= tpe_sequence_init(tpe);

	tpe->obj   	= tpe_obj_init(tpe);
	tpe->orders  	= tpe_orders_init(tpe);
	tpe->board 	= tpe_board_init(tpe);
	tpe->gui	= tpe_gui_init(tpe);
	tpe->ship	= tpe_ship_init(tpe);

	tpe->ai 	= ai_smith_init(tpe);

	ecore_main_loop_begin();

	return 0;
}


static void
parse_args(int argc, char **argv){
	int i,j;

	for (i = 1 ; i < argc ; i ++){
		for (j = 0 ; j < sizeof(args)/sizeof(args[0]) ; j ++)	
			if (!strncmp(args[j].arg, argv[i],strlen(args[j].arg)))
				i = args[j].fn(i,argv);
	}

}



int 
parse_username(int i, char **args){
	return 0;	

}
