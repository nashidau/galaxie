#include <inttypes.h>
#include <stdlib.h>

#include <Ecore_Data.h>

#include "tpe.h"
#include "tpe_obj.h"
#include "tpe_util.h"

struct object *
ai_util_planet_closest_uncolonised(struct tpe *tpe, struct object *obj){
	Ecore_List *all;
	struct object *dest, *cur;
	uint64_t destdist, curdist;

	all = tpe_obj_obj_list(tpe->obj);
	
	ecore_list_goto_first(all);
	dest = NULL;
	destdist = UINT64_MAX;
	while ((cur = ecore_list_next(all))){
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