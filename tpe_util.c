#include <arpa/inet.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ecore_Data.h>

#include "tpe_util.h"

#include "tpe_msg.h" /* for the htonll */
#include "tpe_obj.h" /* Only for types */
#include "tpe_orders.h" /* Only for types */

/**
 * tpe_util_string_extract
 *
 * Extracts a string (in byte swapped format), and returns a pointer to first
 * byte after
 */
char *
tpe_util_string_extract(const char *src, int *lenp, const char **endp){
	int len;
	char *buf;
	len = ntohl(*(int *)src);
	if (endp) *endp = src + len + 4;
	if (lenp) *lenp = len;
	buf = malloc(len + 1);
	strncpy(buf,src + 4, len);
	buf[len] = 0;
	return buf;
}


/*
 * Raw dump of the packet to standard out
 * Warning: May be big
 *
 */
char *
tpe_util_dump_packet(void *pdata){
	char *p;

	p = pdata;
	printf("%c%c%c%c %d %d %d\n",
			p[0],p[1],p[2],p[3],
			ntohl(*(int *)(p + 4)),
			ntohl(*(int *)(p + 8)),
			ntohl(*(int *)(p + 12)));

	return 0;
}


/**
 * tpe_util_parse_packet: Parses a packet into the specified data pointers.
 *
 * FIXME: Need a more general way of parsing complex structures (lists)
 *
 * Format string:
 *  s: A string - will be malloced into pointer
 *  i: int - 32 bit int
 *  l: long long int - 64 bit int
 *  a: Array of ints
 *  O: Array of Oids
 *  S: Array of ships
 *  R: Array of Planet resources 
 *  B: Arrya of build resouts { int resource id, #required }
 *  Q: Array of order description args
 *  6: Arg type 6: An array of { int, string, int }
 *  p: Save a pointer to the current offset
 */
int
tpe_util_parse_packet(void *pdata, char *format, ...){
	int len;
	int parsed;
	va_list ap;
	
	len = ntohl(*((int *)pdata + 3));
	parsed = 0;

	va_start(ap,format);
	
	while (*format){
		switch (*format){
			case 'i':{ /* Single 32 bit int */
				 int val;
				 int *idata;
				 int *dest;

				 dest = va_arg(ap,int *);	

				 idata = pdata;
				 val = ntohl(*idata);
				 idata ++;

				 if (dest) *dest = val;

				 pdata = idata;
				 format ++;
				 parsed ++;
				 break;
			 }
			case 'l':{ /* Single 64 bit int */
				 int64_t val;
				 int64_t *idata;
				 int64_t *dest;

				 dest = va_arg(ap,int64_t *);	

				 idata = pdata;
				 val = *idata;
				 val = ntohll(val);
				 idata ++;

				 if (dest) *dest = val;

				 pdata = idata;
				 format ++;
				 parsed ++;
				 break;
			 }
			case 's':{ /* string */
				char **dest;
				char *sval;

				dest = va_arg(ap, char **);

				sval = tpe_util_string_extract(pdata, 
					NULL, (void *)&pdata);
				if (dest) *dest = sval;

				format ++;
				parsed ++;
				break;
			}
			case 'a':{ /* Array of ints */
				int **adest;
				int *cdest;
				int *idata;
				int len;
				int i;

				idata = pdata;
				len = ntohl(*idata);
				idata ++;
			
				cdest = va_arg(ap, int *);
				adest = va_arg(ap, int **);

				if (cdest) *cdest = len;

				if (adest){
					*adest = realloc(*adest, (len + 1)* sizeof(int));
					for (i = 0 ; i < len ; i ++){
						(*adest)[i] = ntohl(*idata ++);
					}
				} else {
					idata += len;
				}

				pdata = (char *)idata;
				format ++;
				parsed ++;
				break;
			}
			case 'O':{
				struct ObjectSeqID **adest;
				int len;
				int *idata;
				int64_t tmp;
				int *cdest;
				int i;
				
				idata = pdata;
				len = ntohl(*idata);
				idata++;

				cdest = va_arg(ap, int *);
				adest = va_arg(ap, struct ObjectSeqID **);

				if (cdest) *cdest = len;
	
				*adest = realloc(*adest, (len+1)*
						sizeof(struct ObjectSeqID));
				
				for (i = 0 ; i < len ; i ++){
					(*adest)[i].oid = ntohl(*idata ++);
					memcpy(&tmp,idata,8);
					(*adest)[i].updated = ntohll(tmp);
					idata += 2;
				}

				pdata = (char *)idata;
				format ++;
				parsed ++;
				break;

			}
			case 'p':{
				int **adest;
				format ++;
				adest = va_arg(ap, int **);
				if (adest == NULL){
					break;
				} 
				*adest = pdata;
				break;
			}
			/* Array of order arguments */
			case 'Q':{
				struct order_arg **adest;
				int len;
				int *idata;
				int *cdest;
				int i;
				
				idata = pdata;
				len = ntohl(*idata);
				idata++;

				cdest = va_arg(ap, int *);
				adest = va_arg(ap, struct order_arg **);

				if (cdest) *cdest = len;
	
				*adest = realloc(*adest, (len+1)*
						sizeof(struct order_arg));

				for (i = 0 ; i < len ; i ++){
					(*adest)[i].name = 
						tpe_util_string_extract(idata, 
								NULL, 
								(void *)&idata);
					(*adest)[i].arg_type = ntohl(*idata ++);
					(*adest)[i].description = 
						tpe_util_string_extract(idata, 
								NULL, 
								(void *)&idata);
				}

				pdata = (char *)idata;
				format ++;
				parsed ++;
				break;
			}
			/* Arg type '6'
			 * 	An array returned when querying orders */
			case '6':{
				struct arg_type6 **adest;
				int len;
				int *idata;
				int *cdest;
				int i;
				
				idata = pdata;
				len = ntohl(*idata);
				idata++;

				cdest = va_arg(ap, int *);
				adest = va_arg(ap, struct arg_type6 **);

				if (cdest) *cdest = len;
	
				*adest = realloc(*adest, (len+1)*
						sizeof(struct arg_type6));

				for (i = 0 ; i < len ; i ++){
					(*adest)[i].id = ntohl(*idata ++);
					(*adest)[i].name = 
						tpe_util_string_extract(idata, 
								NULL, 
								(void *)&idata);
					(*adest)[i].max = ntohl(*idata ++);
				}

				pdata = (char *)idata;
				format ++;
				parsed ++;
				break;
			}
			/* Arg type '6'
			 * 	An array returned when querying orders */
			case 'B':{
				struct build_resources **adest;
				int len;
				int *idata;
				int *cdest;
				int i;
				
				idata = pdata;
				len = ntohl(*idata);
				idata++;

				cdest = va_arg(ap, int *);
				adest = va_arg(ap, struct arg_type6 **);

				if (cdest) *cdest = len;
	
				*adest = realloc(*adest, (len+1)*
						sizeof(struct build_resources));

				for (i = 0 ; i < len ; i ++){
					(*adest)[i].rid = ntohl(*idata ++);
					(*adest)[i].cost = ntohl(*idata ++);
				}

				pdata = (char *)idata;
				format ++;
				parsed ++;
				break;
			}
			/* Array of resources */
			case 'R':{
				struct planet_resource **adest;
				int len;
				int *idata;
				int *cdest;
				int i;
				
				idata = pdata;
				len = ntohl(*idata);
				idata++;

				cdest = va_arg(ap, int *);
				adest = va_arg(ap, struct planet_resource **);

				if (cdest) *cdest = len;
	
				*adest = realloc(*adest, (len+1)*
						sizeof(struct planet_resource));
				
				for (i = 0 ; i < len ; i ++){
					(*adest)[i].rid = ntohl(*idata ++);
					(*adest)[i].surface = ntohl(*idata ++);
					(*adest)[i].minable = ntohl(*idata ++);
					(*adest)[i].inaccessable = 
							ntohl(*idata ++);
				}

				pdata = (char *)idata;
				format ++;
				parsed ++;
				break;
			}

			/* Array of ships */
			case 'S':{
				struct fleet_ship **adest;
				int len;
				int *idata;
				int *cdest;
				int i;
				
				idata = pdata;
				len = ntohl(*idata);
				idata++;

				cdest = va_arg(ap, int *);
				adest = va_arg(ap, struct fleet_ship **);

				if (cdest) *cdest = len;
	
				*adest = realloc(*adest, (len+1)*
						sizeof(struct fleet_ship));
				
				for (i = 0 ; i < len ; i ++){
					(*adest)[i].design = ntohl(*idata ++);
					(*adest)[i].count = ntohl(*idata ++);
					idata += 2;
				}

				pdata = (char *)idata;
				format ++;
				parsed ++;
				break;
			}
			default: 
				printf("Unhandled code %c\n",*format);
				return parsed;
		}
	}


	return parsed;
}

/*
 * Returns distance between two objexct squared.
 *
 * FIXME: Should check for overflow.
 */
uint64_t 
tpe_util_dist_calc2(struct object *obj1, struct object *obj2){
	uint64_t x,y,z;

	x = obj1->pos.x - obj2->pos.x;
	y = obj1->pos.y - obj2->pos.y;
	z = obj1->pos.z - obj2->pos.z;

	return (uint64_t)hypotl(x,hypotl(y,z));
	//return x * x + y * y + z * z;
}
