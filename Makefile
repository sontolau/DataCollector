TESTAPP=test
LIB=libdc.a
LIBSO=libdc.so
CC=clang
CFLAGS=-fPIC -Wall -g
LIBS=-lev -ljson-c
OBJS=link.o list.o queue.o locker.o object.o keyval.o hash.o buffer.o thread.o
INCDIRS=-I./ -I/usr/include/libev -I/usr/include/json-c

all: ${LIB} ${LIBSO}
${TESTAPP}: ${LIB}
	gcc test.c -o ${TESTAPP} ${INCDIRS} ${LIB} -g
${LIB}:${OBJS}
	ar r ${LIB} ${OBJS} 
${LIBSO}:${OBJS}
	${CC} -shared -Wl,-soname,libdc.so.1 -o ${LIBSO} ${OBJS} ${LIBS}
.c.o:
	${CC} ${CFLAGS} ${INCDIRS} -c $< -o $@ -g ${LIBS}
clean:
	rm -f *~ ${LIBSO} ${LIB} ${OBJS} ${TESTAPP}

install:${LIB}
	if [ ! -d "/usr/include/libdc" ]; then \
		mkdir /usr/include/libdc; \
	fi; \
	install -m 0664 ./*.h /usr/include/libdc; \
	install -m 0644 ${LIB} /usr/lib/
	install -m 0644 ${LIBSO} /usr/lib/
	ldconfig
uninstall:
	if [ -d "/usr/include/libdc" ]; then \
		rm -rf /usr/include/libdc; \
	fi; \
	rm -rf /usr/lib/${LIB}
	rm -rf /usr/lib/${LIBSO}
