
#define INTTOPTR(x) 	((void*)(intptr_t)(x))
#define PTRTOINT(x)	((int)(intptr_t)(x))

#define CHECK_TYPE(obj, typename)                        		\
        ({                                                              \
                Smart_Data *CT_sd;                                      \
                const char *CT_type;                                    \
                if ((obj) == NULL){                                     \
                        fprintf(stderr,"%s:%d Obj is Null\n",           \
                                __FILE__, __LINE__);                    \
                        return;                                         \
                }                                                       \
                CT_sd = evas_object_smart_data_get(obj);                \
                if (CT_sd == NULL) {                                    \
                        fprintf(stderr,"%s:%d SmartData is Null\n",     \
                                __FILE__, __LINE__);                    \
                        return;                                         \
                }                                                       \
                CT_type = evas_object_type_get(obj);                    \
                if (CT_type == NULL ||                                  \
                    strcmp(CT_type,(typename)) != 0) {                  \
                        fprintf(stderr,"%s:%d Type is %s not %s\n",     \
                               __FILE__, __LINE__,                      \
                                CT_type,(typename));                    \
                        return;                                         \
                }                                                       \
		CT_sd;							\
        })


#define CHECK_TYPE_RETURN(obj, typename, rv)                  		\
        ({                                                              \
                Smart_Data *CT_sd;                                      \
                const char *CT_type;                                    \
                if ((obj) == NULL){                                     \
                        fprintf(stderr,"%s:%d Obj is Null\n",           \
                                __FILE__, __LINE__);                    \
                        return (rv);                                    \
                }                                                       \
                CT_sd = evas_object_smart_data_get(obj);                \
                if (CT_sd == NULL) {                                    \
                        fprintf(stderr,"%s:%d SmartData is Null\n",     \
                                __FILE__, __LINE__);                    \
                        return (rv);                                    \
                }                                                       \
                CT_type = evas_object_type_get(obj);                    \
                if (CT_type == NULL ||                                  \
                    strcmp(CT_type,(typename)) != 0) {                  \
                        fprintf(stderr,"%s:%d Type is %s not %s\n",     \
                               __FILE__, __LINE__,                      \
                                CT_type,(typename));                    \
                        return (rv);                                    \
                }                                                       \
		CT_sd;							\
        })

