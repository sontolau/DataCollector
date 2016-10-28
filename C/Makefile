TESTAPP=test
LIBDC=libdc.a libdc.so
CC=cc
CFLAGS=-fPIC -Wall -g
LIBS= #-lev -ljson-c
OBJS=link.o queue.o string.o
INCDIRS=-I./ -I/usr/include/libev -I/usr/include/json-c

all: ${LIBDC}
${TESTAPP}: ${LIB}
	gcc test.c -o ${TESTAPP} ${INCDIRS} ${LIBDC} -g
${LIBDC}:${OBJS}
	ar r libdc.a ${OBJS} 
	${CC} -shared -Wl,-soname,libdc.so.1 -o libdc.so ${OBJS} ${LIBS}
.c.o:
	${CC} ${CFLAGS} ${INCDIRS} -c $< -o $@ -g ${LIBS}
clean:
	rm -f *~ ${LIBDC} ${OBJS} ${TESTAPP}


install:${LIBDC}
	if [ ! -d "/usr/include/libdc" ]; then \
		mkdir /usr/include/libdc; \
	fi; \
	install -m 0664 ./*.h /usr/include/libdc; \
	install -m 0644 ${LIBDC} /usr/lib/
	ldconfig

uninstall:
	if [ -d "/usr/include/libdc" ]; then \
		rm -rf /usr/include/libdc; \
	fi; \
	rm -rf /usr/lib/${LIBDC}
