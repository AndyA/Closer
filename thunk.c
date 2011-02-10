/* thunk.c */

#include <stdio.h>

struct thunk_data {
  const char *name;
  int ( *cl ) ( int x, const char *name );
};

static int
closure( int x, const char *name ) {
  return x * 2;
}

static int
thunk( int x ) {
  struct thunk_data *data = ( ( struct thunk_data * ) thunk ) - 1;
  return data->cl( x, data->name );
}

int
main( void ) {
  return 0;
}

/* vim:ts=2:sw=2:sts=2:et:ft=c 
 */
