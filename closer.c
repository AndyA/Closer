/* BASENAME.c */

/* <skip> */
#include "closer.h"
#include "tap.h"
/* </skip> */
/* <include block="INCLUDE_HEADER" /> */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define SLOTS NSLOTS

struct closure_slot {
  unsigned next;
  unsigned refcount;
  NAME cl;
   RETURN( *code ) ( ALL_PROTO );
  void ( *cleanup ) ( CTX_PROTO );
};

struct closure_data {
  CTX_PROTO_STMT;
};

struct closure_index {
  unsigned long long addr;      /* ordered */
  unsigned idx;                 /* cl_index into closure_data, closure_slot */
};

/* <skip> */
static RETURN closure_0( PASS_PROTO );
static RETURN closure_1( PASS_PROTO );
static RETURN closure_2( PASS_PROTO );
static RETURN closure_3( PASS_PROTO );
/* </skip> */
/* <include block="CLOSURE_PROTOTYPES" /> */

static struct closure_slot cl_slot[SLOTS] = {
/* <skip> */
  {1, 0, closure_0, NULL, NULL},
  {2, 0, closure_1, NULL, NULL},
  {3, 0, closure_2, NULL, NULL},
  {4, 0, closure_3, NULL, NULL},
/* </skip> */
/* <include block="CLOSURE_TABLE" /> */
};

static struct closure_data cl_data[SLOTS];
static struct closure_index cl_index[SLOTS];

static unsigned cl_free_slot = 0;
static unsigned cl_indexed = 0;

#if defined( THREADED_CLOSURES ) || defined( THREADED_NAME )
pthread_mutex_t cl_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cl_try_again = PTHREAD_COND_INITIALIZER;
#define new_closure_cleanup new_NAME_cleanup_nts
#define new_closure         new_NAME_nts
#define free_closure        free_NAME_nts
#define clone_closure       clone_NAME_nts
#else
#define new_closure_cleanup new_NAME_cleanup
#define new_closure         new_NAME
#define free_closure        free_NAME
#define clone_closure       clone_NAME
#endif

/* <skip> */
static RETURN
closure_0( PASS_PROTO ) {
  return cl_slot[0].code( CALL_ARGS( 0 ) );
}

static RETURN
closure_1( PASS_PROTO ) {
  return cl_slot[1].code( CALL_ARGS( 1 ) );
}

static RETURN
closure_2( PASS_PROTO ) {
  return cl_slot[2].code( CALL_ARGS( 2 ) );
}

static RETURN
closure_3( PASS_PROTO ) {
  return cl_slot[3].code( CALL_ARGS( 3 ) );
}

/* </skip> */
/* <include block="CLOSURE_DEFINITIONS" /> */

NAME
new_closure_cleanup( RETURN( *code ) ( ALL_PROTO ), CTX_PROTO,
                     void ( *cleanup ) ( CTX_PROTO ) ) {
  unsigned s;
  if ( cl_free_slot == SLOTS )
    return NULL;
  s = cl_free_slot;
  cl_free_slot = cl_slot[s].next;
  cl_slot[s].next = SLOTS + 1;
  cl_slot[s].refcount = 1;
  cl_slot[s].code = code;
  cl_slot[s].cleanup = cleanup;
  CTX_COPY_STMT;

  return cl_slot[s].cl;
}

NAME
new_closure( RETURN( *code ) ( ALL_PROTO ), CTX_PROTO ) {
  return new_NAME_cleanup( code, CTX_ARGS, NULL );
}

static int
by_addr( const void *a, const void *b ) {
  const struct closure_index *cia = ( const struct closure_index * ) a;
  const struct closure_index *cib = ( const struct closure_index * ) b;
  return cia->addr < cib->addr ? -1 : cia->addr > cib->addr ? 1 : 0;
}

static void
build_index( void ) {
  unsigned i;
  for ( i = 0; i < SLOTS; i++ ) {
    cl_index[i].addr = ( unsigned long long ) cl_slot[i].cl;
    cl_index[i].idx = i;
  }
  qsort( cl_index, SLOTS, sizeof( cl_index[0] ), by_addr );
}

static unsigned
lookup_closure( NAME cl ) {
  if ( !cl_indexed ) {
    build_index(  );
    cl_indexed++;
  }
  struct closure_index key = { ( unsigned long long ) cl, 0 };
  struct closure_index *ix =
      bsearch( &key, cl_index, SLOTS, sizeof( cl_index[0] ), by_addr );
  return ix ? ix->idx : UINT_MAX;
}

static unsigned
lookup_active_closure( NAME cl ) {
  unsigned i = lookup_closure( cl );
  if ( i == UINT_MAX || cl_slot[i].next != SLOTS + 1 ) {
    fprintf( stderr, "Address is not a valid closure" );
    exit( 1 );
  }
  return i;
}

void
free_closure( NAME cl ) {
  if ( cl ) {
    unsigned i = lookup_active_closure( cl );
    if ( --cl_slot[i].refcount == 0 ) {
      if ( cl_slot[i].cleanup ) {
        cl_slot[i].cleanup( CLEANUP_ARGS );
        cl_slot[i].cleanup = NULL;
      }
      cl_slot[i].next = cl_free_slot;
      cl_free_slot = i;
    }
  }
}

NAME
clone_closure( NAME cl ) {
  unsigned i = lookup_active_closure( cl );
  cl_slot[i].refcount++;
  return cl;
}

#if defined( THREADED_CLOSURES ) || defined( THREADED_NAME )
NAME
new_NAME_cleanup( RETURN( *code ) ( ALL_PROTO ), CTX_PROTO,
                  void ( *cleanup ) ( CTX_PROTO ) ) {
  return new_NAME_nb( code, CTX_ARGS, cleanup, 0 );
}

NAME
new_NAME( RETURN( *code ) ( ALL_PROTO ), CTX_PROTO ) {
  return new_NAME_nb( code, CTX_ARGS, NULL, 0 );
}

void
free_NAME( NAME cl ) {
  pthread_mutex_lock( &cl_lock );
  free_closure( cl );
  pthread_mutex_unlock( &cl_lock );
  pthread_cond_signal( &cl_try_again );
}

NAME
clone_NAME( NAME cl ) {
  NAME cl2;
  /* TODO this is a bigger lock than we need; we're only changing a
   * single closure. 
   */
  pthread_mutex_lock( &cl_lock );
  cl2 = clone_closure( cl );
  pthread_mutex_unlock( &cl_lock );
  return cl2;
}

NAME
new_NAME_nb( RETURN( *code ) ( ALL_PROTO ), CTX_PROTO,
             void ( *cleanup ) ( CTX_PROTO ), unsigned timeout ) {
  NAME cl;
  pthread_mutex_lock( &cl_lock );
  if ( cl_free_slot == SLOTS ) {
    if ( timeout == 0 ) {
      pthread_mutex_unlock( &cl_lock );
      return NULL;
    }
    else if ( timeout == UINT_MAX ) {
      pthread_cond_wait( &cl_try_again, &cl_lock );
    }
    else {
      struct timeval tv;
      struct timespec ts;

      gettimeofday( &tv, NULL );

      ts.tv_sec = tv.tv_sec + timeout / 1000;
      ts.tv_nsec = ( tv.tv_usec + ( timeout % 1000 ) * 1000 ) * 1000;
      if ( ts.tv_nsec > 1000000000 ) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec++;
      }

      if ( pthread_cond_timedwait( &cl_try_again, &cl_lock, &ts ) ) {
        pthread_mutex_unlock( &cl_lock );
        return NULL;
      }
    }
  }
  cl = new_closure_cleanup( code, CTX_ARGS, cleanup );
  pthread_mutex_unlock( &cl_lock );
  return cl;
}

#endif

/* <skip> */

static const char *last_h = NULL;
static int last_x = 0;

static RETURN
printer( ALL_PROTO ) {
/*  diag( h, x );*/
  last_h = h;
  last_x = x;
  return x * 2;
}

static void
called_printer_ok( ALL_PROTO, const char *msg ) {
  ok( last_h != NULL && strcmp( last_h, h ) == 0, "%s", msg );
  last_h = NULL;
  last_x = 0;
}

static void
cleanup( CTX_PROTO ) {
/*  diag( "cleaning up %s", h );*/
  last_h = h;
}

static void
called_cleanup_ok( CTX_PROTO, const char *msg ) {
  ok( last_h != NULL && strcmp( last_h, h ) == 0, "%s", msg );
  last_h = NULL;
}

static void
test_basic( void ) {
  NAME cl1 = new_NAME_cleanup( printer, "hello %d", cleanup );
  not_null( cl1, "cl1 - non null" );
  NAME cl2 = new_NAME( printer, "world %d" );
  not_null( cl2, "cl2 - non null" );
  NAME cl3 = clone_NAME( cl1 );
  not_null( cl3, "cl3 - non null" );
  ok( cl1 == cl3, "clone is alias" );
  cl1( 1 );
  called_printer_ok( 1, "hello %d", "cl1(1)" );
  cl1( 2 );
  called_printer_ok( 2, "hello %d", "cl1(2)" );
  cl2( 3 );
  called_printer_ok( 3, "world %d", "cl2(3)" );
  cl2( 4 );
  called_printer_ok( 4, "world %d", "cl2(4)" );
  free_NAME( cl1 );
  null( last_h, "ref count decremented, cleanup not called" );
  cl1 = new_NAME( printer, "whoop %d" );
  not_null( cl1, "cl1 - non null again" );
  cl1( 1 );
  called_printer_ok( 1, "whoop %d", "cl1(1)" );
  cl1( 2 );
  called_printer_ok( 2, "whoop %d", "cl1(2)" );
  cl2( 3 );
  called_printer_ok( 3, "world %d", "cl2(3)" );
  cl2( 4 );
  called_printer_ok( 4, "world %d", "cl2(4)" );
  cl3( 1 );
  called_printer_ok( 1, "hello %d", "cl3(1)" );
  cl3( 2 );
  called_printer_ok( 2, "hello %d", "cl3(2)" );
  free_NAME( cl3 );
  called_cleanup_ok( "hello %d", "cleanup (2)" );
  free_NAME( cl2 );
  free_NAME( cl1 );
}

static void
test_exhaustion( const char *s ) {
  NAME cl[SLOTS];
  char buf[30];
  int i;

  for ( i = 0; i < sizeof( cl ) / sizeof( cl[0] ); i++ ) {
    sprintf( buf, "closure %d %%d", i + 1 );
    cl[i] = new_NAME( printer, buf );
    not_null( cl[i], "%s: closure %d allocated", s, i + 1 );
  }
  null( new_NAME( printer, "failure %d" ), "%s: all allocated", s );
  for ( i = 0; i < sizeof( cl ) / sizeof( cl[0] ); i++ ) {
    free_NAME( cl[i] );
  }
}

int
main( void ) {
  plan( 17 + ( SLOTS + 1 ) * 2 );
  test_basic(  );
  test_exhaustion( "pass 1" );
  test_exhaustion( "pass 2" );
  return 0;
}

/* </skip> */

/* vim:ts=2:sw=2:sts=2:et:ft=c 
 */
