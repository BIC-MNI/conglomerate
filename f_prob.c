#include  <internal_volume_io.h>
#include  <bicpl.h>

private  Real  gamma_func(
    Real   x );

int  main(
    int    argc,
    char   *argv[] )
{
    int    D, m, n, v;
    Real   resels, t, p, t1, t2, t3;

    initialize_argument_processing( argc, argv );

    if( !get_real_argument( 0.0, &resels ) ||
        !get_int_argument( 0, &D ) ||
        !get_int_argument( 0, &m ) ||
        !get_int_argument( 0, &n ) ||
        !get_real_argument( 0.0, &t ) )
    {
        print_error( "Usage: %s  resels D n t\n", argv[0] );
        return( 1 );
    }

    t = t * (1.0 / (Real) m + 1.0 / (Real) n);

    v = n + m - 2 - D - 1;

    t = t * (Real) v / (Real)(m + n - 2) / (Real) D;

    t1 = resels * 4.0 * log(2.0) / 2.0 / PI;
    t2 = gamma_func( (Real) (v+D-2)/2.0 ) /
         gamma_func( (Real) v / 2.0 ) / gamma_func( (Real) D / 2.0 ) *
         pow( D * t / (Real) v, 0.5*(D-2) );
    t3 = pow( 1.0 + D * t / v, -0.5*(v+D-2) ) *
         ((Real)(v-1)*D * t / v - (D-1.0) );
    p = t1 * t2 * t3;

    print( "%g\n", p );

    return( 0 );
}

private  Real  gamma_func(
    Real   x )
{
    Real   lg, g;

    lg = gamma(x); g = signgam*exp(lg);

    return( g );
}
