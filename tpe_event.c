#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore.h>
#include <Ecore_Data.h>

#include "tpe_event.h"


/*
 * Globacl generic TPE event structure */
struct tpe_event {
	struct tpe *tpe;
	Ecore_Hash *hash;
};

struct event_info {
	const char *name;
	int ecore_event;
	int handlers;
};

unsigned int hash_pjw(const char *s);

/**
 * Initialise the event system 
 */
struct tpe_event *
tpe_event_init(struct tpe *tpe){
	struct tpe_event *tpe_ev;

	tpe_ev = calloc(1,sizeof(struct tpe_event));
	if (!tpe_ev) return NULL;

	tpe_ev->tpe = tpe;

	tpe_ev->hash = ecore_hash_new((unsigned int(*)(const void*))hash_pjw, 
			(int(*)(const void*,const void*))strcmp);

	/* Init lazily */

	return tpe_ev;
}


int 
tpe_event_type_add(struct tpe_event *tpeev, const char *name){
	struct event_info *einfo;

	/* Register a new event by this name */
	einfo = calloc(1,sizeof(struct event_info));
	einfo->name = strdup(name);
	einfo->ecore_event = ecore_event_type_new();
	einfo->handlers = 0;
	
	/* XXX: This cast needs to go: Needs a fix in ecore however */
	ecore_hash_set(tpeev->hash, (void*)name, einfo);	
	return 0;
}

/**
 * Adds a handler for the specified event 
 *
 * Checks to make sure the event type has been registered first 
 */
int 
tpe_event_handler_add(struct tpe_event *tpeev, const char *event,
		int (*handler)(void *data, int typeid, void *event), void *data){
	struct event_info *einfo;

	assert(tpeev);

	einfo = ecore_hash_get(tpeev->hash, event);
	if (einfo == NULL){
		printf("Unable to find event '%s'\n",event);
		return -1;
	}

	ecore_event_handler_add(einfo->ecore_event, handler, data);
	einfo->handlers ++;

	return 0;
}



/*
 * hash_pjw() -- hash function from P. J. Weinberger's C compiler
 *              call with NUL-terminated string s.
 *              returns value in range 0 .. HASHTABLE_SIZE-1
 *
 * Reference: Alfred V. Aho, Ravi Sethi, Jeffrey D. Ullman
 *            _Compilers: Principles, Techniques, and Tools_
 *            Addison-Wesley, 1986.
 */

unsigned int
hash_pjw(const char *s)
{
        const char *p;
        unsigned int h, g;

        h = 0;

        for (p = s; *p != '\0'; p++) {
                h = (h << 4) + *p;
                g = h & 0xf0000000;
                if (g != 0) {
                        h ^= g >> 24;
                        h ^= g;
                }
        }
        return h;
}




/*
 * Sends an event of the specified type to the system.
 *
 * The event must have been registered for the vent to be sent.
 */
int
tpe_event_send(struct tpe_event *tpeev, const char *event, void *edata,	
		void (*freefn)(void *data, void *event), void *freedata){
	struct event_info *einfo;

	assert(tpeev);

	einfo = ecore_hash_get(tpeev->hash, event);
	if (einfo == NULL){
		printf("Warning: Unregisted event '%s'\n",event);
		return -1;
	}

	if (freefn == NULL && edata != NULL){
		printf("Warning: tpe_event_send with NULL freefn.\n");
		printf("\tThis doesn't do what you think it does.\n");
		printf("\tEvent is `%s'\n",event);
	}


	if (einfo->handlers)
		ecore_event_add(einfo->ecore_event, edata, freefn, freedata);
	else if (freefn)
		freefn(freedata, edata);
	else 
		free(edata);

	return 0;
}


void 
tpe_event_nofree(void *x,void *y){

}
void 
tpe_event_free(void *x,void *y){
	free(y);
}
