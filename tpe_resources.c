/**
 * @file tpe_resources.h
 *
 * TPE Resources provides a base class for dealing with resources.  It
 * currently implements resource descriptions (through the struct
 * resourcedescription).  
 *
 * A resource description shows the basic information about a resource
 * available from the server.
 *
 * FIXME: Should be an event for a new type of resource
 *
 */
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_event.h"
#include "tpe_resources.h"
#include "tpe_sequence.h"
#include "tpe_util.h"

enum {
	RESOURCE_MAGIC = 0x4428f32b,
};

struct tpe_resources {
	uint32_t magic;
        Ecore_List *resources;
};


static int tpe_resources_resourcedescription_msg(void *,int,void*);

/**
 * Initialise the resource system. 
 *
 * This function should be called at start up time.  It registers the resource 
 * description handlers.
 *
 * @param tpe TPE structure
 * @return tpe_resources structure or NULL on error
 */
struct tpe_resources *
tpe_resources_init(struct tpe *tpe){
	struct tpe_resources *resources;

	resources = calloc(1,sizeof(struct tpe_resources));
	resources->magic = RESOURCE_MAGIC;
	resources->resources = ecore_list_new();

	tpe_event_type_add(tpe->event, "ResourceNew");

	tpe_sequence_register(tpe, "MsgGetResourceDescriptionIDs",
                        "MsgListOfResourceDescriptionsIDs",
                        "MsgGetResourceDescription",
                        tpe_resources_resourcedescription_updated,
                        NULL, NULL);

        tpe_event_handler_add(tpe->event, "MsgResourceDescription",
                        tpe_resources_resourcedescription_msg, tpe);



        return resources;
}

/**
 * Finds a resourcedescription by ID
 *
 * @param tpe TPE structure
 * @param resourceid Resource id to search for
 * @return Resouce Description, or NULL on error
 */
struct resourcedescription *
tpe_resources_resourcedescription_get(struct tpe *tpe, uint32_t resourceid){
        struct resourcedescription *r;
	assert(tpe->resources->magic == RESOURCE_MAGIC);
        ecore_list_first_goto(tpe->resources->resources);
        while ((r = ecore_list_next(tpe->resources->resources)))
                if (r->id == resourceid)
                        return r;

        return NULL;

}

/**
 * Get the updated time on a particular resource.  
 *
 * @param tpe TPE structure
 * @param resourceid Resource id to search for
 * @return The last time resource was updated, or 0 if not found.
 */
uint64_t
tpe_resources_resourcedescription_updated(struct tpe *tpe, uint32_t resourceid){
        struct resourcedescription *r;

	assert(tpe->resources->magic == RESOURCE_MAGIC);
        r = tpe_resources_resourcedescription_get(tpe,resourceid);
        if (r)
                return r->updated;
        return 0;
}


/**
 * Event handler for a resource description
 *
 * Format of the resource message is: (From Tp03)
 *
 * - a UInt32, Resource ID
 * - a Formatted String, singular name of the resource
 * - a Formatted String, plural name of the resource
 * - a Formatted String, singular name of the resources unit
 * - a Formatted String, plural name of the resources unit
 * - a Formatted String, description of the resource
 * - a UInt32, weight per unit of resource (0 for not applicable)
 * - a UInt32, size per unit of resource (0 for not applicable)
 * - a UInt64, the last modified time of this resource description
 */
 
static int 
tpe_resources_resourcedescription_msg(void *data,int etype,void *event){
	struct resourcedescription *rd;
	struct tpe *tpe;
	int *msg;
	uint32_t id;
	int rv;

	tpe = data;
	msg = event;

	assert(tpe->resources->magic == RESOURCE_MAGIC);
	msg += 4;
	
	tpe_util_parse_packet(msg,NULL, "i", &id);

	rd = tpe_resources_resourcedescription_get(tpe, id);
	if (rd == NULL){
		rd = calloc(1,sizeof(struct resourcedescription));
		rd->id = id;
		ecore_list_append(tpe->resources->resources,rd);
		/* Can queue now... not sent until we return... */
		tpe_event_send(tpe->event, "ResourceNew", rd, 
				tpe_event_nofree, NULL);
	}

	/* FIXME: Check we got it all */
	rv = tpe_util_parse_packet(msg, NULL, "-sssssiil",
		&rd->name, &rd->name_plural,
		&rd->unit, &rd->unit_plural,
		&rd->description,
		&rd->weight, &rd->size,
		&rd->updated);

	if (rv != 9){
		printf("Did not get all parameters for resource description\n");
		printf("Got %d, expected %d\n",rv,9);
		printf("Resource is %s (%s)\n",rd->name,rd->name_plural);
		return 1;
	}

	{ int printf(const char *,...);
	printf("New resource: %s\n",rd->name);
	}

	return 1;
}

/** FIXME: Document */
uint32_t 
tpe_resources_resourcedescription_get_by_name(struct tpe *tpe, 
		const char *name){
	struct resourcedescription *r;
	assert(tpe);
	assert(tpe->resources);
	assert(name);

	if (tpe == NULL || tpe->resources == NULL || name == name)
		return (uint32_t)-1;
	assert(tpe->resources->magic == RESOURCE_MAGIC);

        ecore_list_first_goto(tpe->resources->resources);
        while ((r = ecore_list_next(tpe->resources->resources))){
		if (r->name == NULL){
			printf("Warning NULL name for resource\n");
		}
                if (strcmp(r->name, name) == 0)
                        return r->id;
	}

	return (uint32_t)-1;
}

