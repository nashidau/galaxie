#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "tpe_util.h"

#include "galaxietypes.h"

/*
0: Vector
	int64_t 	x,y,z;
	objtype_t	relative_to
*/
static struct parseitem rvector[] =  {
	{ PARSETYPE_LONG, offsetof(struct rvector, x), 0, NULL, NULL, 0},
	{ PARSETYPE_LONG, offsetof(struct rvector, y), 0, NULL, NULL, 0},
	{ PARSETYPE_LONG, offsetof(struct rvector, z), 0, NULL, NULL, 0},
	{ PARSETYPE_INT,  offsetof(struct rvector, id), 0, NULL, NULL, 0},
};

/*
1: Velocity
	int64_t		dx,dy,dz;
	objtype_t	relative_to
*/

/*
2: Acceleration
	int64_t		dx,dy,dz;
	objtype_t	relative_to
*/

/*
3: Bound Position
	?? Should there be a object?
	int32_t		slot
*/
static struct parseitem boundposition[] = {
	{ PARSETYPE_INT, 0, 0, NULL, NULL, 0},
};


/*
4: Order Queue:
	uint32_t	maxslots
	uint32_t	queueid
	uint32_t	nordersinqueue
	list
		int32_t	ordertypes
*/
static struct parseitem orderqueue[] = {
	{ PARSETYPE_INT, offsetof(struct orderqueue, max), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, offsetof(struct orderqueue, id), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, offsetof(struct orderqueue, norders), 0, NULL, NULL, 0},
	{ PARSETYPE_ARRAYOF | PARSETYPE_INT, 
			offsetof(struct orderqueue, ordertypes), 
			offsetof(struct orderqueue, nordertypes),
			NULL, NULL, 0 },
};


/*
5: Resource List
	list
		uint32_t	resourceid
		uint32_t	stored
		uint32_t 	minable
		uint32_t 	unavailable	
*/
static struct parseitem pl_resources[] = {
	{ PARSETYPE_INT, offsetof(struct resource, id), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, offsetof(struct resource, stored), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, offsetof(struct resource, minable), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, offsetof(struct resource, unavail), 0, NULL, NULL, 0},
};

/* FIXME: This doesn't belong here */
struct reslist {
	int nresources;
	struct resource *resources;
};

static struct parseitem reslist[] = {
	{ PARSETYPE_COMPLEX, offsetof(struct reslist, resources), 
			offsetof(struct reslist, nresources),
			pl_resources, "struct resource", 
			sizeof(struct resource) },
};


/*
6: Reference
	int32_t		type
	uint32_t	id
*/
static struct parseitem pl_reference[] = {
	{ PARSETYPE_INT, offsetof(struct reference, type), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, offsetof(struct reference, value), 0, NULL, NULL, 0},
};

/*
7: Reference Quantity List
	list
		int32_t 	type
		uint32_t	id
		uint32_t 	number
*/
static struct parseitem pl_refquantity[] = {
	{ PARSETYPE_INT, offsetof(struct refquantity, type), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, offsetof(struct refquantity, value), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, offsetof(struct refquantity, quantity), 0, NULL, NULL, 0},
};

/* FIXME: This doesn't belong here either */
struct refqlist {
	int nrefquantities;
	struct refquantity *refquantities;
};

static struct parseitem pl_referencequantitylist[] = {
	{ PARSETYPE_COMPLEX, offsetof(struct refqlist, refquantities), 
			offsetof(struct refqlist, nrefquantities),
			pl_refquantity, "struct refquanty", 
			sizeof(struct refquantity) },
};



/*
8: Integer
	int32_t		value
*/
static struct parseitem pl_int32[] = {
	{ PARSETYPE_INT, 0, 0, NULL, NULL, 0},
};

/*
9: Size
	int64_t		diameter
*/
static struct parseitem pl_int64[] = {
	{ PARSETYPE_LONG, 0, 0, NULL, NULL, 0},
};

/*
10: Media URI
	(readonly) char *url
*/
static struct parseitem pl_string[] = {
	{ PARSETYPE_STRING, 0, 0, NULL, NULL, 0},
};


