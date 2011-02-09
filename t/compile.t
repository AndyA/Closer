#!/bin/sh

echo "1..2"
perl ./closer.pl t/data/t1.cl \
  && echo "ok 1 - closer.pl" \
  || echo "not ok 1 - closer.pl"
gcc -c -o t1_closure.o t1_closure.c \
  && echo "ok 2 - compile" \
  || echo "not ok 2 - compile"
rm -f t1_closure.*

# vim:ts=2:sw=2:et:ft=sh

