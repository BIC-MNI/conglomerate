#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  Real  evaluate_probability_t(
    int    v,
    Real   t )
{
    Real   gamma_top, gamma_bottom, top, bottom, p;

    gamma_top = exp( gamma( ((Real) v + 1.0) / 2.0 ) );
    gamma_bottom = exp( gamma( (Real) v / 2.0 ) );

    top = gamma_top * pow( 1.0 + t * t / (Real) v, - (Real) (v+1)/ 2.0 );
    bottom = sqrt( (Real) v * PI ) * gamma_bottom;

    p = top / bottom;

    return( p );
}

int  main(
    int  argc,
    char *argv[] )
{
    int   degrees_freedom;
    Real  t, p;

    initialize_argument_processing( argc, argv );

    (void) get_int_argument( 8, &degrees_freedom );
    (void) get_real_argument( 1.0, &t );

    p = evaluate_probability_t( degrees_freedom, t );

    print( "%g: p < %g\n", t, p );

    return( 0 );
}
