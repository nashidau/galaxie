PKGCONFIG=pkg-config
PKGS='evas ecore edje'

CFLAGS+=`${PKGCONFIG} --cflags ${PKGS}`
LDFLAGS+=`${PKGCONFIG} --libs ${PKGS}`


OBJECTS=		\
	tpe.o		\
	tpe_board.o	\
	tpe_comm.o	\
	tpe_event.o	\
	tpe_gui.o	\
	tpe_msg.o	\
	tpe_obj.o	\
	tpe_orders.o	\
	tpe_resources.o	\
	tpe_sequence.o	\
	tpe_ship.o	\
	tpe_util.o	\

AIS=			\
	ai_smith.o	\
	ai_util.o

BASICTHEME=			\
	edje/basic.edc		\
	edje/basic-message.edc  \
	edje/basic-stars.edc


EDJE=	edje/basic.edj 
EDJE_FLAGS=-id edje/images


TARGETS: tpe ${EDJE}

%.edj : %.edc
	edje_cc ${EDJE_FLAGS} $<

tpe: ${OBJECTS} ${AIS}

doc:
	doxygen Doxygen.conf

sparse: 
	sparse -Wall *.c

edje/basic.edj: ${BASICTHEME}


tpe_msg.o : tpe_msg.h tpe.h
ai_smith.o: tpe_obj.h tpe.h tpe_event.h tpe_msg.h tpe_orders.h tpe_ship.h \
		tpe_util.h

clean: 
	rm -f *.o 

