PKGCONFIG=pkg-config
PKGS='evas ecore edje'

CFLAGS+=`${PKGCONFIG} --cflags ${PKGS}`
LDFLAGS+=`${PKGCONFIG} --libs ${PKGS}`

TARGETS: tpe ${EDJE}

OBJECTS=	\
	tpe.o	\
	tpe_comm.o	\
	tpe_event.o	\
	tpe_gui.o	\
	tpe_msg.o	\
	tpe_obj.o	\
	tpe_util.o	\


EDJE=	edje/basic.edj edje/background.edj
EDJE_FLAGS=-id edje/images

%.edj : %.edc
	edje_cc ${EDJE_FLAGS} $<

tpe: ${OBJECTS} 

edje/basic.edj : edje/basic.edc


tpe_msg.o : tpe_msg.h tpe.h

clean: 
	rm -f *.o 

