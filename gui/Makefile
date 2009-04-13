PKGCONFIG=pkg-config
PKGS='evas ecore ecore-evas ecore-con ecore-job edje imlib2 lua5.1 talloc ecore-txt'

.PHONY: tags

XLIBS=-lX11 -lXp -lXext -lXss -lXrandr -lXfixes -lXdamage -lXcursor \
		-lXcomposite


DATAPATH=${PREFIX}/share/galaxie

CFLAGS+=`${PKGCONFIG} --cflags ${PKGS}` -DDATAPATH=\"${DATAPATH}\"
LDFLAGS+=${XLIBS} `${PKGCONFIG} --libs ${PKGS}`


targets: teststar testmap

teststar:teststar.o test.o star.o ../tpe_event.o
	${CC} -o $@ $^ ${LDFLAGS}

testmap:testmap.o test.o map.o star.o ../tpe_event.o
	${CC} -o $@ $^ ${LDFLAGS}

tags:
	ctags -R .
	cscope -R -b


clean:
	rm -f *.o teststar

#---- Dependancies ----#
star.o: star.h widgetsupport.h
teststar.o: star.h test.h
map.o: star.h map.h widgetsupport.h
testmap.o: map.h star.h test.h map.o
