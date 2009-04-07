PKGCONFIG=pkg-config
PKGS='evas ecore ecore-con ecore-job edje imlib2 lua5.1 talloc'

CFLAGS+=`${PKGCONFIG} --cflags ${PKGS}`
LDFLAGS+=`${PKGCONFIG} --libs ${PKGS}`

ifeq "${PREFIX}" ""
	PREFIX=/usr/local
endif

DATADIR=${PREFIX}/share/galaxie/

# Image path
I=edje/images/

OBJECTS=		\
	galaxie.o	\
	browser.o	\
	server.o	\
	tpe_board.o	\
	tpe_event.o	\
	tpe_obj.o	\
	object_param.o	\
	tpe_orders.o	\
	tpe_resources.o	\
	tpe_sequence.o	\
	tpe_ship.o	\
	tpe_util.o	\
	tpe_comm.o	\
	ai_util.o

GUI=	#		\
	tpe_gui.o	\
	tpe_gui_orders.o	\
	gui_window.o	\
	gui_list.o	

NEWGUI=	gui/map.o	\
	gui/star.o

AIS=			\
	ai_smith.o	\
	ai_jones.o

AI_SRCS=${subst .o,.c,${AIS}}

BASICTHEME=			\
	edje/basic.edc		\
	edje/basic-info.edc	\
	edje/basic-menu.edc	\
	edje/basic-message.edc	\
	edje/basic-orders.edc	\
	edje/basic-refs.edc	\
	edje/basic-ships.edc	\
	edje/basic-stars.edc	\
	edje/basic-window.edc

IMAGES=				\
	$Iarrowright.png	\
	$Iarrowleft.png		\
	$Ibg.png		\
	$Ibutton.png		\
	$Imailbox.png		\
	$Iclose.png		\
	$Imessagewindow.png	\
	$Iobjectwindow.png	\
	$Iwindow.png


EDJE=edje/basic.edj 
EDJE_FLAGS=-id edje/images


TARGETS: galaxie ${EDJE}

%.edj : %.edc
	edje_cc ${EDJE_FLAGS} $<

# Big FIXME: install edje data, and make sure binaries can find it
install:
	cp galaxie ${PREFIX}/bin

ailist.h:  ${AI_SRCS}
	grep -h TPE_AI ${AI_SRCS} > ailist.h

galaxie: ${OBJECTS} ${GUI} ${AIS} 
	${CC} -o galaxie ${OBJECTS} ${GUI} ${AIS} ${LDFLAGS}

doc:
	doxygen Doxygen.conf

sparse: 
	sparse -Wall *.c

testedje: ${EDJE}
	edje ${EDJE}

edje/basic.edj: ${BASICTHEME} ${IMAGES} Makefile

tags:
	cscope -R -b -I/usr/local/include 			\
		-s.						\
		-s/home/nash/work/e17--cvs/libs/evas/src/lib 	\
		-s/home/nash/work/e17--cvs/libs/edje/src/lib	\
		-s/home/nash/work/e17--cvs/libs/ecore/src/lib


galaxie.o: ailist.h tpe_orders.h
ai_smith.o: tpe_obj.h tpe.h tpe_event.h server.h tpe_orders.h tpe_ship.h \
		tpe_util.h
ai_jones.o : tpe.h tpe_orders.h
tpe_gui.o: tpe.h tpe_gui.h tpe_board.h tpe_comm.h tpe_event.h tpe_obj.h \
		tpe_orders.h tpe_ship.h tpe_util.h tpe_reference.h \
		tpe_gui_private.h
tpe_gui_orders.o: tpe.h tpe_gui_private.h tpe_orders.h
ai_util.o : tpe.h
gui_window.o : tpe.h tpe_gui.h tpe_gui_private.h
gui_list.o : tpe.h tpe_gui.h gui_window.h tpe_orders.h
tpe_board.o : tpe.h tpe_board.h tpe_util.h
tpe_comm.o : tpe.h tpe_comm.h
tpe_obj.o : tpe_obj.h tpe.h tpe_orders.h tpe_util.h
tpe_orders.o : tpe_orders.h tpe.h
tpe_resources.o : tpe_resources.h tpe.h
tpe_sequence.o: tpe.h tpe_event.h tpe_sequence.h server.h tpe_util.h
tpe_ship.o : tpe.h tpe_event.h server.h tpe_util.h tpe_sequence.h tpe_ship.h
tpe_util.o : tpe.h tpe_orders.h tpe_util.h 
ewl/gewl_object.o : tpe.h server.h tpe_orders.h

$Iarrowright.png : $Imailviewer.svg
	inkscape -j --export-id=path5138 --export-png $@ $<

$Iarrowleft.png : $Imailviewer.svg
	inkscape -j --export-id=use5148 --export-png $@ $<

$Imessagewindow.png : $Imailviewer.svg
	inkscape -j --export-id=${@F} --export-png $@ $<

$Imailbox.png : $Imailbox.svg
	inkscape -j --export-id=${@F} --export-png $@  $<

$Ibg.png : $IBG1.svg
	inkscape -j --export-background=black --export-png $@ $<

$Iobjectwindow.png : $IObjectViewer.svg
	inkscape -j --export-id=${@F} -d 120 --export-png $@ $<

$Ibutton.png : $IObjectViewer.svg
	inkscape -j --export-id=${@F} -w 200 -h 50 --export-png $@ $<

$Iclose.png : $IObjectViewer.svg
	inkscape -j --export-id=${@F} -w 200 -h 50 --export-png $@ $<

$Iwindow.png : $Iwindow.svg
	inkscape -j --export-area-drawing -w 300 -h 400 --export-png $@ $<

fixme: 
	egrep \(FIXME\|XXX\) *.c **/*.c

clean: 
	rm -f *.o */*.o ailist.h ${EDJE}

reallyclean: 
	rm -f *.o */*.o ailist.h ${EDJE} ${IMAGES}

veryclean: clean
	rm -f  ${IMAGES}

