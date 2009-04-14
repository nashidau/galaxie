#include <inttypes.h>
#include <stdlib.h>

#include <Ecore_Data.h>

#include "tpe.h"
#include "galaxie.h"
#include "tpe_obj.h"
#include "tpe_util.h"
#include "ai_util.h"

struct object *
ai_util_planet_closest_uncolonised(struct tpe *tpe, struct object *obj){
	Ecore_List *all;
	struct object *dest, *cur;
	uint32_t oid;
	uint64_t destdist, curdist;

	all = tpe_obj_obj_list(tpe->obj);
	
	ecore_list_first_goto(all);
	dest = NULL;
	destdist = UINT64_MAX;
	while ((oid = (int)ecore_list_next(all))){
		cur = tpe_obj_obj_get_by_id(tpe,oid);
		if (cur->type != OBJTYPE_PLANET) continue;
		if (dest == NULL && cur->ai == NULL) {
			dest = cur;
			destdist = tpe_util_dist_calc2(dest,obj);
			continue;
		}
		if (cur->ai != NULL) continue;
		if (cur->owner >= 0) continue;
		curdist = tpe_util_dist_calc2(cur,obj);
		if (curdist < destdist){
			/* New target */
			destdist = curdist;
			dest = cur;
		}
	}

	return dest;
}
