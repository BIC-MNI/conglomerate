#include <stdio.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/time.h>
#include <malloc.h>

int  quicksort_threshold = 13;

#define  ALLOC( ptr, n ) \
      (ptr) = malloc( sizeof(*(ptr)) * (n) )

#define  FREE( ptr ) \
      free( (void *) ptr )

typedef  unsigned long  sort_type;

static  double  get_cpu_seconds()
{
    double        seconds;
    struct   tms  buffer;

    (void) times( &buffer );

    seconds = (double) buffer.tms_utime / (double) HZ;

    return( seconds );
}

static  void  simple_sort(
    int       n,
    sort_type list[] );
static  void  quick_sort(
    int       n,
    sort_type list[] );
static  void  fast_sort(
    int       n,
    sort_type list[] );

int   main(
    int   argc,
    char  *argv[] )
{
    int              n_iters = 1;
    int              n = 10000;
    int              i, iter, debug, which = 0;
    sort_type        *values;
    double           start, end;
    unsigned  long   seed;
    struct  timeval  t;

    if( argc > 1 )
        (void) sscanf( argv[1], "%d", &n );

    if( argc > 2 )
        (void) sscanf( argv[2], "%d", &which );

    if( argc > 3 )
        (void) sscanf( argv[3], "%d", &quicksort_threshold );

    if( argc > 4 )
        (void) sscanf( argv[4], "%d", &n_iters );

    debug = argc > 5;

    (void) gettimeofday( &t, (struct timezone *) 0 );

    seed = t.tv_usec + t.tv_sec;
    (void) srand( seed );

    ALLOC( values, n );

    start = get_cpu_seconds();

    for( iter = 0;  iter < n_iters;  ++iter )
    {
        for( i = 0;  i < n;  ++i )
            values[i] = rand();

        switch( which )
        {
        case 0:
            simple_sort( n, values );
            break;
        case 1:
            quick_sort( n, values );
            break;
        case 2:
            fast_sort( n, values );
            break;
        }

        if( debug )
        {
            for( i = 0;  i < n-1;  ++i )
                if( values[i] > values[i+1] )
                    break;

            if( i != n-1 )
                (void) printf( "Failed\n" );
        }
    }

    end = get_cpu_seconds();

    FREE( values );

    (void) printf( "%g seconds = %g / iter\n", (end - start),
                   (end - start) / (double) n_iters );

    return( 0 );
}

#define  SWAP( a, b, type ) \
    { \
        type  _tmp; \
        _tmp = (a); \
        (a) = (b); \
        (b) = _tmp; \
    }

static  void  simple_sort(
    int       n,
    sort_type list[] )
{
    int         i, j, smallest;

    if( n < 2 )
        return;
    else if( n == 2 )
    {
        if( list[0] > list[1] )
            SWAP( list[0], list[1], sort_type )
    }
    else
        for( i = 0;  i < n-1;  ++i )
        {
            smallest = i;
            for( j = i + 1;  j < n;  ++j )
            {
                if( list[j] < list[smallest] )
                    smallest = j;
            }

            SWAP( list[smallest], list[i], sort_type )
        }
    }
}

static  void  quick_sort(
    int       n,
    sort_type list[] )
{
    int         bottom, top, i;
    sort_type   partition;

    if( n <= quicksort_threshold )
        simple_sort( n, list );
    else
    {
        i = 1;
        while( i < n && list[i] == list[0] )
            ++i;
        if( list[0] < list[i] )
            partition = list[i];
        else
            partition = list[0];

        bottom = 0;
        top = n-1;

        while( bottom < top )
        {
            while( bottom < n && list[bottom] < partition )
                ++bottom;
            while( top >= 0 && list[top] >= partition )
                --top;
            if( bottom < top )
            {
                SWAP( list[bottom], list[top], sort_type )
            }
        }

        if( list[0] != list[n-1] )
        {
            quick_sort( bottom, list );
            quick_sort( n - bottom, &list[bottom] );
        }
    }
}

#define  N_VALUES  65536

static  void  shifted_sort(
    int       n,
    sort_type list[],
    int       n_shift,
    sort_type new_list[] )
{
    int    i, val, save_pos, current_pos;
    int    counts[N_VALUES];

    for( i = 0;  i < N_VALUES;  ++i )
        counts[i] = 0;

    for( i = 0;  i < n;  ++i )
    {
        val = (list[i] >> n_shift) & (N_VALUES-1);
        ++counts[val];
    }

    current_pos = 0;
    for( i = 0;  i < N_VALUES;  ++i )
    {
        save_pos = current_pos;
        current_pos += counts[i];
        counts[i] = save_pos;
    }

    for( i = 0;  i < n;  ++i )
    {
        val = (list[i] >> n_shift) & (N_VALUES-1);
        new_list[counts[val]] = list[i];
        ++counts[val];
    }
}

static  void  fast_sort(
    int       n,
    sort_type list[] )
{
    int       i;
    sort_type *new_list;

    ALLOC( new_list, n );

    shifted_sort( n, list, 0, new_list );
    shifted_sort( n, new_list, 16, list );
/*
    shifted_sort( n, new_list, 8, list );
    shifted_sort( n, list, 16, new_list );
    shifted_sort( n, new_list, 24, list );
*/

    for( i = 0;  i < n;  ++i )
        list[i] = new_list[i];

    FREE( new_list );
}
