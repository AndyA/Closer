/* BASENAME.c */

/* <skip> */
#include "closer.h"
/* </skip> */
/* <include block="INCLUDE_HEADER" /> */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
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
  unsigned idx;                 /* index into closure_data, closure_slot */
};

/* <skip> */
static RETURN closure_0( PASS_PROTO );
static RETURN closure_1( PASS_PROTO );
/* </skip> */
/* <include block="CLOSURE_PROTOTYPES" /> */

static struct closure_slot slot[SLOTS] = {
/* <skip> */
  {1, 0, closure_0, NULL, NULL},
  {2, 0, closure_1, NULL, NULL},
/* </skip> */
/* <include block="CLOSURE_TABLE" /> */
};

static struct closure_data data[SLOTS];
static struct closure_index index[SLOTS];

static unsigned free_slot = 0;
static unsigned indexed = 0;

#if defined( THREADED_CLOSURES ) || defined( THREADED_NAME )
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t try_again = PTHREAD_COND_INITIALIZER;
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
  return slot[0].code( CALL_ARGS( 0 ) );
}

static RETURN
closure_1( PASS_PROTO ) {
  return slot[1].code( CALL_ARGS( 1 ) );
}

/* </skip> */
/* <include block="CLOSURE_DEFINITIONS" /> */

NAME
new_closure_cleanup( RETURN( *code ) ( ALL_PROTO ), CTX_PROTO,
                     void ( *cleanup ) ( CTX_PROTO ) ) {
  unsigned s;
  if ( free_slot == SLOTS )
    return NULL;
  s = free_slot;
  free_slot = slot[s].next;
  slot[s].next = SLOTS + 1;
  slot[s].refcount = 1;
  slot[s].code = code;
  slot[s].cleanup = cleanup;
  CTX_COPY_STMT;

  return slot[s].cl;
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
    index[i].addr = ( unsigned long long ) slot[i].cl;
    index[i].idx = i;
  }
  qsort( index, SLOTS, sizeof( index[0] ), by_addr );
}

static unsigned
lookup_closure( NAME cl ) {
  if ( !indexed ) {
    build_index(  );
    indexed++;
  }
  struct closure_index key = { ( unsigned long long ) cl, 0 };
  struct closure_index *ix =
      bsearch( &key, index, SLOTS, sizeof( index[0] ), by_addr );
  return ix ? ix->idx : UINT_MAX;
}

static unsigned
lookup_active_closure( NAME cl ) {
  unsigned i = lookup_closure( cl );
  if ( i == UINT_MAX || slot[i].next != SLOTS + 1 ) {
    fprintf( stderr, "Address is not a valid closure" );
    exit( 1 );
  }
  return i;
}

void
free_closure( NAME cl ) {
  unsigned i = lookup_active_closure( cl );
  if ( --slot[i].refcount == 0 ) {
    if ( slot[i].cleanup ) {
      slot[i].cleanup( CLEANUP_ARGS );
      slot[i].cleanup = NULL;
    }
    slot[i].next = free_slot;
    free_slot = i;
  }
}

NAME
clone_closure( NAME cl ) {
  unsigned i = lookup_active_closure( cl );
  slot[i].refcount++;
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
  pthread_mutex_lock( &lock );
  free_closure( cl );
  pthread_mutex_unlock( &lock );
  pthread_cond_signal( &try_again );
}

NAME
clone_NAME( NAME cl ) {
  NAME cl2;
  /* TODO this is a bigger lock than we need; we're only changing a
   * single closure. 
   */
  pthread_mutex_lock( &lock );
  cl2 = clone_closure( cl );
  pthread_mutex_unlock( &lock );
  return cl2;
}

NAME
new_NAME_nb( RETURN( *code ) ( ALL_PROTO ), CTX_PROTO,
             void ( *cleanup ) ( CTX_PROTO ), unsigned timeout ) {
  NAME cl;
  pthread_mutex_lock( &lock );
  if ( free_slot == SLOTS ) {
    /* TODO do something other than wait forever if the timeout != 0 */
    if ( timeout == 0 ) {
      pthread_mutex_unlock( &lock );
      return NULL;
    }
    else if ( timeout == UINT_MAX ) {
      pthread_cond_wait( &try_again, &lock );
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

      if ( pthread_cond_timedwait( &try_again, &lock, &ts ) ) {
        pthread_mutex_unlock( &lock );
        return NULL;
      }
    }
  }
  cl = new_closure_cleanup( code, CTX_ARGS, cleanup );
  pthread_mutex_unlock( &lock );
  return cl;
}

#endif

/* <skip> */

static RETURN
printer( ALL_PROTO ) {
  printf( ( char * ) h, x );
  printf( "\n" );
  return x * 2;
}

static void
cleanup( CTX_PROTO ) {
  printf( "cleaning up %s\n", ( char * ) h );
}

int
main( void ) {
  NAME cl1 = new_NAME_cleanup( printer, ( void * ) "hello %d", cleanup );
  NAME cl2 = new_NAME( printer, ( void * ) "world %d" );
  cl1( 1 );
  cl1( 2 );
  cl2( 3 );
  cl2( 4 );
  free_NAME( cl1 );
  cl1 = new_NAME( printer, ( void * ) "whoop %d" );
  cl1( 1 );
  cl1( 2 );
  cl2( 3 );
  cl2( 4 );
  return 0;
}

/* </skip> */

/* vim:ts=2:sw=2:sts=2:et:ft=c 
 */
