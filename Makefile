# Author: Jérémie Galarneau <jeremie.galarneau@gmail.com>
#
# Who licences a Makefile... Do whatever you want...

PYTHON_INCLUDE_DIR = $(shell python3-config --includes)
PYTHON_FLAGS = $(shell python3-config --ldflags)
LIBS = -lbabeltrace -lbabeltrace-ctf

CPPFLAGS += $(PYTHON_INCLUDE_DIR)
LDFLAGS += $(PYTHON_FLAGS)
CFLAGS += -Wall

all: _swig_adapter.so filtertest

swig_adapter.c: swig_adapter.i
	swig -python -o $@ swig_adapter.i

swig_adapter.o: swig_adapter.c
	$(CC) -fpic $(CPPFLAGS) $(CFLAGS) $(AM_CPPFLAGS) -c -o $@ $<

_swig_adapter.so: swig_adapter.o
	$(CC) -shared -o $@ $?

filtertest.o: filtertest.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(AM_CPPFLAGS) -c -o $@ $<

filtertest: filtertest.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ filtertest.o $(LIBS)

.PHONY: clean
clean:
	rm -f *.o *.so *~ swig_adapter.c swig_adapter.py swig_adapter.o swig_adapter.so filtertest
