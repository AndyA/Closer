#!/bin/sh

echo "1..3"
perl ./closer.pl t/data/t1.cl \
  && echo "ok 1 - closer.pl" \
  || echo "not ok 1 - closer.pl"
gcc -Wall -Werror -c -o t1_closure.o t1_closure.c \
  && echo "ok 2 - compile" \
  || echo "not ok 2 - compile"
gcc -Wall -Werror -DTHREADED_CLOSURES -c -o t1_closure.o t1_closure.c \
  && echo "ok 3 - threaded compile" \
  || echo "not ok 3 - threaded compile"
rm -f t1_closure.*

# vim:ts=2:sw=2:et:ft=sh

