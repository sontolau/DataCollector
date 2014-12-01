TESTAPP=test
LIB=libDC.a
CC=cc
CFLAGS=-fPIC -Wall -g
<<<<<<< HEAD
OBJS=link.o list.o hash.o dict.o memory.o queue.o
=======
OBJS=link.o list.o hash.o dict.o number.o signal.o
>>>>>>> 106bc725c08cc347d613535ca0f520f13d986b1e
INCDIRS=-I./

all: ${LIB}
${TESTAPP}: ${LIB}
	gcc test.c -o ${TESTAPP} ${INCDIRS} ${LIB}
${LIB}:${OBJS}
	ar r ${LIB} ${OBJS}
.c.o:
	${CC} ${CFLAGS} ${INCDIRS} -c $< -o $@
clean:
	rm -f ${LIB} ${OBJS} ${TESTAPP}
