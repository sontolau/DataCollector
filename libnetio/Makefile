LIBNETIO=libnetio.so libnetio.a
LIBS=-lpthread -lgnutls -lev
#LIBPATH=-L./ -L../libdc
APPS= tcpserver tcpsender
OBJS=N_io.o N_netkit.o
CC=cc
FLAGS=-Wall -fPIC -g
INCDIRS=-I./ -I../

all:${LIBNETIO} ${APPS}
${LIBNETIO}: ${OBJS}
	${CC} -shared  -o libnetio.so ${OBJS}
	ar r libnetio.a ${OBJS}
${APPS}:${LIBNETIO}
	${CC} ${FLAGS} ${INCDIRS} tcpserver.c -o tcpserver ./libnetio.a ../libdc/libdc.a ${LIBS}
	${CC} ${FLAGS} ${INCDIRS} tcpsender.c -o tcpsender ./libnetio.a ../libdc/libdc.a ${LIBS}
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
