#include  <stdio.h>

int  main(
    int   argc,
    char  *argv[] )
{
    unsigned long   colour;
    unsigned long   r, g, b, a;

    if( argc < 2 )
    {
        (void) printf( "Argument.\n" );
        return( 1 );
    }

    (void) sscanf( argv[1], "%d", &colour );

    r = colour & 255ul;
    g = (colour >> 8ul) & 255ul;
    b = (colour >> 16ul) & 255ul;
    a = (colour >> 24ul) & 255ul;

    (void) printf( "%d %d %d %d\n", r, g, b, a );

    return( 0 );
}
