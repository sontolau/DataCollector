LIB=libpnet.so
OBJS=netsrv.o netcomm.o
CFLAGS=-Wall -fPIC -g
CC=gcc
LIBS=-lpthread -lDC -lev
INCDIRS=-I/usr/include/libev

${LIB}:${OBJS}
	${CC} -shared ${OBJS} -o ${LIB} ${CFLAGS} ${LIBS}
.c.o:
	${CC} -c $< -o $@ ${INCDIRS} ${CFLAGS}
clean:
	rm -rf ${LIB} ${OBJS}
install: ${LIB}
	if [ ! -d "/usr/include/libpnet" ]; then \
		mkdir /usr/include/libpnet ; \
	fi;\
	install -m 0664 ./netsrv.h /usr/include/libpnet;\
	install -m 0755 ${LIB} /usr/lib/;\
