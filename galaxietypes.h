
typedef unsigned int objid_t;

/* A vector relative to something */
struct rvector {
	int64_t x,y,z;
	objid_t id;
};

struct orderqueue {
	uint32_t max;
	uint32_t id;
	uint32_t norders;
	
	uint32_t nordertypes;
	int32_t	*ordertypes;
};


struct resourcelist { 
	int nresources;
	struct resources *resources;
};

struct resource {
	uint32_t id;
	uint32_t stored;
	uint32_t minable;
	uint32_t unavail;
};


struct refquantitylist {
	uint32_t nrefquantities;
	struct refquantity *refquantities;
};
struct refquantity { 
	int32_t type;
	uint32_t value;
	uint32_t quantity;
};


