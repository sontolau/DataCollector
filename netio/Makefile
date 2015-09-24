LIBNETIO=libnetio.so
OBJS=N_io.o N_netkit.o
CC=clang
FLAGS=-Wall -fPIC
INCDIRS=

${LIBNETIO}: ${OBJS}
	${CC} -shared -Wl -o ${LIBNETIO} ${OBJS}
.c.o:
	${CC} ${FLAGS} ${INCDIRS} -c $< -o $@
clean:
	rm ${OBJS} ${LIBNETIO} *~
