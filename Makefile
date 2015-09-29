LIBS=libdc libnetio
#all: ${LIBS}
all:
	cd libdc && make
	cd libnetio && make
install: ${all}
	cd libdc && make install
	cd libnetio && make install
clean:
	cd libdc && make clean
	cd libnetio && make clean
