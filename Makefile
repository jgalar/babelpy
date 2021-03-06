# Author: Jérémie Galarneau <jeremie.galarneau@gmail.com>
#
# Who licences a Makefile... Do whatever you want...

PYTHON_INCLUDE_DIR = $(shell python3-config --includes)
PYTHON_FLAGS = $(shell python3-config --ldflags)
LIBS = -lbabeltrace -lbabeltrace-ctf

CPPFLAGS += $(PYTHON_INCLUDE_DIR)
LDFLAGS += $(PYTHON_FLAGS)
CFLAGS += -Wall

all: _swig_adapter.so babelpy

swig_adapter.c: swig_adapter.i
	swig -python -o $@ swig_adapter.i

swig_adapter.o: swig_adapter.c
	$(CC) -fpic $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

_swig_adapter.so: swig_adapter.o
	$(CC) -shared -o $@ $?

babelpy.o: babelpy.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

babelpy: babelpy.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)

.PHONY: clean
clean:
	rm -f *.o *.so *~ swig_adapter.c swig_adapter.py swig_adapter.o swig_adapter.so babelpy
