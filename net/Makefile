LIB=libpnet.so
STLIB=libpnet.a
OBJS=netsrv.o netcomm.o
CFLAGS=-Wall -fPIC -g
CC=gcc
LIBS=-lpthread -lev
INCDIRS=-I/usr/include/libev

all:${LIB} ${STLIB}
${LIB}:${OBJS}
	${CC} -shared ${OBJS} -o ${LIB} ${CFLAGS} ${LIBS} -lDC
${STLIB}:${OBJS}
	ar r ${STLIB} ${OBJS}
.c.o:
	${CC} -c $< -o $@ ${INCDIRS} ${CFLAGS} ../libDC.a
clean:
	rm -rf ${LIB} ${OBJS} ${STLIB}
install: ${LIB}
	if [ ! -d "/usr/include/libpnet" ]; then \
		mkdir /usr/include/libpnet ; \
	fi;\
	install -m 0664 ./netsrv.h /usr/include/libpnet;\
	install -m 0755 ${LIB} /usr/lib/;\
	install -m 0755 ${STLIB} /usr/lib/
