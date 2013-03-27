#include  <bicpl.h>

int main(
    int   argc,
    char  *argv[] )
{
    VIO_Real   x, y, z, h, s, l,  r, g, b;

    initialize_argument_processing( argc, argv );

    (void) get_real_argument( 0.0, &x );
    (void) get_real_argument( 0.0, &y );
    (void) get_real_argument( 0.0, &z );

    hsl_to_rgb( x, y, z, &r, &g, &b );
    print( "RGB:  %g %g %g\n", r, g, b );

    rgb_to_hsl( x, y, z, &h, &s, &l );
    print( "HSL:  %g %g %g\n", h, s, l );

    return( 0 );
}
