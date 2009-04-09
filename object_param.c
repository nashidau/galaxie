#include <stdio.h>
#include <stdint.h>

#include "tpe_util.h"

#include "object_param.h"

extern struct orderqueue _orderqueue;
extern struct reference _reference;
extern struct refquantitylist _refqlist;
extern struct refquantity _refquantity;
extern struct resource _resource;
extern struct resourcelist _reslist;
extern struct rvector _rvector;

#define OFFSET(x,field) 	((char*)&x.field - (char*)&x)


/*
0: Vector
	int64_t 	x,y,z;
	objtype_t	relative_to
*/
static struct parseitem rvector[] =  {
	{ PARSETYPE_LONG, OFFSET(_rvector, x), 0, NULL, NULL, 0},
	{ PARSETYPE_LONG, OFFSET(_rvector, y), 0, NULL, NULL, 0},
	{ PARSETYPE_LONG, OFFSET(_rvector, z), 0, NULL, NULL, 0},
	{ PARSETYPE_INT,  OFFSET(_rvector, id), 0, NULL, NULL, 0},
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
	{ PARSETYPE_INT, OFFSET(_orderqueue, max), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, OFFSET(_orderqueue, id), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, OFFSET(_orderqueue, norders), 0, NULL, NULL, 0},
	{ PARSETYPE_ARRAYOF | PARSETYPE_INT, 
			OFFSET(_orderqueue, ordertypes), 
			OFFSET(_orderqueue, nordertypes),
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
	{ PARSETYPE_INT, OFFSET(_resource, id), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, OFFSET(_resource, stored), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, OFFSET(_resource, minable), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, OFFSET(_resource, unavail), 0, NULL, NULL, 0},
};

static struct parseitem reslist[] = {
	{ PARSETYPE_COMPLEX, OFFSET(_reslist, resources), 
			OFFSET(_reslist, nresources),
			pl_resources, "struct resource", 
			sizeof(struct resource) },
};


/*
6: Reference
	int32_t		type
	uint32_t	id
*/
static struct parseitem pl_reference[] = {
	{ PARSETYPE_INT, OFFSET(_reference, type), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, OFFSET(_reference, value), 0, NULL, NULL, 0},
};

/*
7: Reference Quantity List
	list
		int32_t 	type
		uint32_t	id
		uint32_t 	number
*/
static struct parseitem pl_refquantity[] = {
	{ PARSETYPE_INT, OFFSET(_refquantity, type), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, OFFSET(_refquantity, value), 0, NULL, NULL, 0},
	{ PARSETYPE_INT, OFFSET(_refquantity, quantity), 0, NULL, NULL, 0},
};



static struct parseitem pl_referencequantitylist[] = {
	{ PARSETYPE_COMPLEX, OFFSET(_refqlist, refquantities), 
			OFFSET(_refqlist, nrefquantities),
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


