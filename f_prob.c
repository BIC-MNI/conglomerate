#include  <volume_io.h>
#include  <bicpl.h>

private  Real  gamma_func(
    Real   x );

int  main(
    int    argc,
    char   *argv[] )
{
    int    D, m, n;
    Real   resels, t, p, t1, t2, t3, v, nu;

    initialize_argument_processing( argc, argv );

    if( !get_real_argument( 0.0, &resels ) ||
        !get_int_argument( 0, &D ) ||
        !get_int_argument( 0, &m ) ||
        !get_int_argument( 0, &n ) ||
        !get_real_argument( 0.0, &t ) )
    {
        print_error( "Usage: %s  resels D m n t\n", argv[0] );
        return( 1 );
    }

    v = (Real) (n + m - 2);
    nu = v - (Real) D + 1.0;

    t1 = resels * 4.0 * log(2.0) / 2.0 / PI;
    t2 = gamma_func( (nu+(Real)D-2.0)/2.0 ) /
         gamma_func( nu / 2.0 ) / gamma_func( (Real) D / 2.0 ) *
         pow( (Real) D * t / nu, 0.5*(Real) (D-2) );
    t3 = pow( 1.0 + (Real) D * t / nu, -0.5*(nu+(Real)D-2.0) ) *
         ((nu-1.0)*(Real) D * t / nu - ((Real) D-1.0) );
    p = t1 * t2 * t3;

    print( "%g\n", p );

    return( 0 );
}

private  Real  gamma_func(
    Real   x )
{
    Real   lg, g;

    lg = gamma(x);
    g = (Real) signgam * exp( lg );

    return( g );
}
