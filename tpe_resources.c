struct tpe_resources {
        Ecore_List *resources;
};
struct resourcedescription {
        uint32_t rid;
	char *name;
	char *name_plural;
	char *unit;
	char *unit_plura;
	char *description;
	uint32_t weight;
	uint32_t size;
        uint64_t updated;
};




struct tpe_resource *
tpe_resource_init(struct tpe *tpe){
	struct tpe_resource *resources;

	resources = calloc(1,sizeof(struct resources));
	resources->resources = ecore_list_new();

	tpe_sequence_register(tpe, "MsgGetResourceDescriptionIDs",
                        "MsgListOfResourceDescriptionsIDs",
                        "MsgGetResourceDescription",
                        tpe_resource_resourcedescription_updated,
                        NULL, NULL);

        tpe_event_handler_add(event, "MsgResourceDescription",
                        tpe_ship_msg_design, tpe);

        return ships;
}


struct resourcedescription *
tpe_resource_resourcedescription_get(struct tpe *tpe, uint32_t design){
        struct resourcedescription *r;
        ecore_list_goto_first(tpe->resources->resources);
        while ((r = ecore_list_next(tpe->resources->resources)))
                if (r->rid == resource)
                        return r;

        return NULL;

}


uint64_t
tpe_resource_resourcedescription_updated(struct tpe *tpe, uint32_t resource){
        struct resourcedescription *r;

        r = tpe_resource_resourcedescription_get(tpe,design);
        if (r)
                return r->updated;
        return 0;
}

