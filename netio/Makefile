LIBNETIO=libnetio.so
LIBNETIO_S=libnetio.a
LIBS=-lpthread -lgnutls -lev ${LIBNETIO_S} ../libdc.a
LIBPATH=-L./ -L../
APPS=udpserver udpsender tcpserver tcpsender
OBJS=N_io.o N_netkit.o
CC=clang
FLAGS=-Wall -fPIC -g
INCDIRS=

all:${LIBNETIO} ${LIBNETIO_S} ${APPS}
${LIBNETIO_S}: ${OBJS}
	ar r ${LIBNETIO_S} ${OBJS}
${LIBNETIO}: ${OBJS}
	${CC} -shared -Wl -o ${LIBNETIO} ${OBJS}
${APPS}:${LIBNETIO_S}
	${CC} udpserver.c ${LIBPATH} ${LIBS} -o udpserver
	${CC} udpsender.c ${LIBPATH} ${LIBS} -o udpsender
	${CC} tcpserver.c ${LIBPATH} ${LIBS} -o tcpserver
	${CC} tcpsender.c ${LIBPATH} ${LIBS} -o tcpsender
.c.o:
	${CC} ${FLAGS} ${INCDIRS} -c $< -o $@
clean:
	rm -rf ${OBJS} ${LIBNETIO} ${LIBNETIO_S} ${APPS} *~
