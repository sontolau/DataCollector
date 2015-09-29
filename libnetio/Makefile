LIBNETIO=libnetio.so libnetio.a
LIBS=-lpthread -lgnutls -lev ${LIBNETIO} -ldc
LIBPATH=-L./ -L../libdc
APPS=udpserver udpsender tcpserver tcpsender
OBJS=N_io.o N_netkit.o
CC=clang
FLAGS=-Wall -fPIC -g
INCDIRS=-I./ -I../

all:${LIBNETIO} ${APPS}
${LIBNETIO}: ${OBJS}
	${CC} -shared -Wl -o libnetio.so ${OBJS}
	ar r libnetio.a ${OBJS}
${APPS}:${LIBNETIO}
	${CC} udpserver.c ${LIBPATH} ${INCDIRS} ${LIBS} -o udpserver
	${CC} udpsender.c ${LIBPATH} ${INCDIRS} ${LIBS} -o udpsender
	${CC} tcpserver.c ${LIBPATH} ${INCDIRS} ${LIBS} -o tcpserver
	${CC} tcpsender.c ${LIBPATH} ${INCDIRS} ${LIBS} -o tcpsender
.c.o:
	${CC} ${FLAGS} ${INCDIRS} -c $< -o $@
clean:
	rm -rf ${OBJS} ${LIBNETIO} ${APPS} *~
install: ${LIBNETIO}
	if [ ! -d "/usr/include/libnetio" ]; then\
		mkdir /usr/include/libnetio;\
	fi
	install -m 0644 *.h /usr/include/libnetio/
	install -m 0644 ${LIBNETIO} /usr/lib/
	ldconfig
