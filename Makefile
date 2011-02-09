CC =  gcc
# Use OPTIMIZE="-g3" for debug build
# Use OPTIMIZE="-fprofile-arcs -ftest-coverage" for gcov build
OPTIMIZE=-O3
DEFINES=-DTHREADED_CLOSURES
CFLAGS = $(DEFINES) $(OPTIMIZE) -W -Wall -I/usr/local/include -L/usr/local/lib -I/opt/local/include -L/opt/local/lib

all: closure

closure.o: closure.h

closure: closure.o
	$(CC) $(CFLAGS) $< -o $@

clean:  
	rm -f *.o tags *.gcda *.gcno *.gcov *.o *.out

.PHONY: tags
tags:
	ctags *.c *.h

test: all
	prove -r t
