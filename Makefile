TESTAPP=test
LIB=libDC.a
CC=cc
CFLAGS=-fPIC -Wall -g

OBJS=link.o list.o hash.o dict.o memory.o queue.o signal.o
INCDIRS=-I./

all: ${LIB} ${TESTAPP}
${TESTAPP}: ${LIB}
	gcc test.c -o ${TESTAPP} ${INCDIRS} ${LIB}
${LIB}:${OBJS}
	ar r ${LIB} ${OBJS} 
.c.o:
	${CC} ${CFLAGS} ${INCDIRS} -c $< -o $@
clean:
	rm -f ${LIB} ${OBJS} ${TESTAPP}
