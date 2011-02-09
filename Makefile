.PHONY: all clean dist install tags

INCLUDES=-I/opt/local/include
LIBS=-L/opt/local/lib -lpthread
# Use OPTIMIZE="-g3" for debug build
# Use OPTIMIZE="-fprofile-arcs -ftest-coverage" for gcov build
OPTIMIZE=-O3
DEFINES=-DTHREADED_CLOSURES
LDFLAGS=$(LIBS)
CPPFLAGS=-Wall $(INCLUDES) $(OPTIMIZE) $(DEFINES)
OBJS=closer.o tap.o
PROG=closer
INSTALL_PREFIX=/usr/local

all: $(PROG)

$(PROG): $(OBJS)

clean:
	rm -rf $(OBJS) $(PROG) *.dSYM $(PROG)-*.tar.gz \
		tags perltags *.gcda *.gcno *.gcov *.out

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
	ptags --sort *.pl > perltags

test: all
	prove t/*.t
