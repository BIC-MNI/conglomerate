#include  <mni.h>

#define   THREE_D
#undef   THREE_D

#ifdef  THREE_D
#define   T( X ) X
#else
#define   T( X )
#endif

#ifdef THREE_D
#define  SIZE   100
#else
#define  SIZE   1000
#endif

#define  N_ITERS   10


int  main(
    int   argc,
    char  *argv[] )
{
    int     x, y, z, i, size;
    int     **T(*)p, *single_p;
    int     static_p[SIZE][SIZE]T([SIZE]);
    Real    start, end;

    start = current_cpu_seconds();

    if( argc > 10 )
        (void) scanf( "%d", &size );
    else
        size = SIZE;

#ifdef  THREE_D
    ALLOC3D( p, size, size, size );
    ALLOC( single_p, size * size * size );
#else
    ALLOC2D( p, size, size );
    ALLOC( single_p, size * size );
#endif

    for_less( i, 0, N_ITERS )
    for_less( y, 0, size )
    {
        for_less( x, 1, size )
        {
#ifdef  THREE_D
            for_less( z, 0, size )
            {
#endif
                static_p[x][y]T([z]) = static_p[x-1][y]T([z]);
#ifdef  THREE_D
            }
#endif
        }
    }

    end = current_cpu_seconds();

    print_time( "static array %g %s\n", end - start );

    start = current_cpu_seconds();

    for_less( i, 0, N_ITERS )
    for_less( y, 0, size )
    {
        {
        for_less( x, 1, size )
#ifdef  THREE_D
            for_less( z, 0, size )
            {
#endif
                p[x][y]T([z]) = p[x-1][y]T([z]);
#ifdef  THREE_D
            }
#endif
        }
    }

    end = current_cpu_seconds();

    print_time( "Multidim array %g %s\n", end - start );

    start = current_cpu_seconds();

    for_less( i, 0, N_ITERS )
    for_less( y, 0, size )
    {
        for_less( x, 1, size )
        {
#ifdef  THREE_D
            for_less( z, 0, size )
            {
                single_p[IJK(x,y,z,size,size)] = single_p[IJK(x-1,y,z,size,size)];
            }
#else
            single_p[IJ(x,y,size)] = single_p[IJ(x-1,y,size)];
#endif
        }
    }

    end = current_cpu_seconds();

    print_time( "Single dim array %g %s\n", end - start );

    return( 0 );
}
