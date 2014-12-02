TESTAPP=test
LIB=libDC.a
CC=cc
CFLAGS=-fPIC -Wall -g

OBJS=link.o list.o hash.o dict.o  queue.o signal.o
INCDIRS=-I./

all: ${LIB} ${TESTAPP}
${TESTAPP}: ${LIB}
	gcc test.c -o ${TESTAPP} ${INCDIRS} ${LIB} -g
${LIB}:${OBJS}
	ar r ${LIB} ${OBJS} 
.c.o:
	${CC} ${CFLAGS} ${INCDIRS} -c $< -o $@ -g
clean:
	rm -f ${LIB} ${OBJS} ${TESTAPP}

install:${LIB}
	if [ ! -d "/usr/include/libdc" ]; then \
		mkdir /usr/include/libdc; \
	fi; \
	install -m 0664 ./*.h /usr/include/libdc; \
	install -m 0644 ${LIB} /usr/lib/

uninstall:
	if [ -d "/usr/include/libdc" ]; then \
		rm -rf /usr/include/libdc; \
	fi; \
	rm -rf /usr/lib/${LIB}