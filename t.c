#include <stdio.h>


int  main()
{
    double  x, prev_x;

    prev_x = 1.0;
    x = 2.0;

    while( x != prev_x )
    {
        prev_x = x;
        x *= 10.0;
    }

    (void) printf( "%g\n", x );

    return( 0 );
}
