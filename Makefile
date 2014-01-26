TESTAPP=test
LIB=libDC.a
CC=cc
CFLAGS=-fPIC -Wall -g
OBJS=link.o list.o hash.o dict.o number.o
INCDIRS=-I./

all: ${TESTAPP} ${LIB}
${TESTAPP}: ${LIB}
	gcc test.c -o ${TESTAPP} ${INCDIRS} ${LIB}
${LIB}:${OBJS}
	ar r ${LIB} ${OBJS}
.c.o:
	${CC} ${CFLAGS} ${INCDIRS} -c $< -o $@
clean:
	rm -f ${LIB} ${OBJS} ${TESTAPP}
