.PHONY: all clean dist install tags

INCLUDES=-I/opt/local/include
LIBS=-L/opt/local/lib -lpthread
# Use OPTIMIZE="-g3" for debug build
# Use OPTIMIZE="-fprofile-arcs -ftest-coverage" for gcov build
OPTIMIZE=-O3
DEFINES=-DTHREADED_CLOSURES
LDFLAGS=$(LIBS)
CPPFLAGS=-Wall -Werror $(INCLUDES) $(OPTIMIZE) $(DEFINES)
OBJS=closer.o tap.o
PROG=closer
INSTALL_PREFIX=/usr/local

all: $(PROG) asm

$(PROG): $(OBJS)

clean:
	rm -rf $(OBJS) $(PROG) *.dSYM $(PROG)-*.tar.gz \
		tags perltags *.gcda *.gcno *.gcov *.out \
		*.s *.dis *.o

dist:
	VERSION=$$(perl -ne '/^#define\s+VERSION\s+\"(.+?)\"/ \
		&& print $$1' $(PROG).c) && \
		DIR=$(PROG)-$$VERSION && \
		mkdir $$DIR && \
		cp *.c Makefile $$DIR && \
		tar zcf $$DIR.tar.gz $$DIR && \
		rm -rf $$DIR

install: $(PROG)
	mkdir -p $(INSTALL_PREFIX)/bin
	cp $(PROG) $(INSTALL_PREFIX)/bin

tags:
	ctags *.c *.h
	which ptags > /dev/null && ptags --sort *.pl > perltags

test: all
	prove t/*.t

asm: thunk_x86_64.s thunk_x86_64.dis thunk_i386.s thunk_i386.dis

thunk_x86_64.s: thunk.c
	gcc -arch x86_64 -S -o thunk_x86_64.s thunk.c

thunk_x86_64.dis thunk_x86_64.hex: thunk.c
	gcc -arch x86_64 -o thunk_x86_64.o thunk.c
	otool -t -v thunk_x86_64.o > thunk_x86_64.asm
	otool -t thunk_x86_64.o > thunk_x86_64.hex
	perl tools/hexmix.pl thunk_x86_64.asm thunk_x86_64.hex > thunk_x86_64.dis

thunk_i386.s: thunk.c
	gcc -arch i386 -S -o thunk_i386.s thunk.c

thunk_i386.dis thunk_i386.hex: thunk.c
	gcc -arch i386 -o thunk_i386.o thunk.c
	otool -t -v thunk_i386.o > thunk_i386.asm
	otool -t thunk_i386.o > thunk_i386.hex
	perl tools/hexmix.pl thunk_i386.asm thunk_i386.hex > thunk_i386.dis
