#include  <volume_io.h>
#include  <bicpl.h>

private  VIO_Real  evaluate_probability_t(
    int    v,
    VIO_Real   t )
{
    VIO_Real   gamma_top, gamma_bottom, top, bottom, p;

    gamma_top = exp( gamma( ((VIO_Real) v + 1.0) / 2.0 ) );
    gamma_bottom = exp( gamma( (VIO_Real) v / 2.0 ) );

    top = gamma_top * pow( 1.0 + t * t / (VIO_Real) v, - (VIO_Real) (v+1)/ 2.0 );
    bottom = sqrt( (VIO_Real) v * PI ) * gamma_bottom;

    p = top / bottom;

    return( p );
}

int  main(
    int  argc,
    char *argv[] )
{
    int   degrees_freedom;
    VIO_Real  t, p;

    initialize_argument_processing( argc, argv );

    (void) get_int_argument( 8, &degrees_freedom );
    (void) get_real_argument( 1.0, &t );

    p = evaluate_probability_t( degrees_freedom, t );

    print( "%g: p < %g\n", t, p );

    return( 0 );
}
