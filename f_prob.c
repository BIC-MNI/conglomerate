#include  <volume_io.h>
#include  <bicpl.h>

static  VIO_Real  gamma_func(
    VIO_Real   x );

int  main(
    int    argc,
    char   *argv[] )
{
    int    D, m, n;
    VIO_Real   resels, t, p, t1, t2, t3, v, nu;

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

    v = (VIO_Real) (n + m - 2);
    nu = v - (VIO_Real) D + 1.0;

    t1 = resels * 4.0 * log(2.0) / 2.0 / PI;
    t2 = gamma_func( (nu+(VIO_Real)D-2.0)/2.0 ) /
         gamma_func( nu / 2.0 ) / gamma_func( (VIO_Real) D / 2.0 ) *
         pow( (VIO_Real) D * t / nu, 0.5*(VIO_Real) (D-2) );
    t3 = pow( 1.0 + (VIO_Real) D * t / nu, -0.5*(nu+(VIO_Real)D-2.0) ) *
         ((nu-1.0)*(VIO_Real) D * t / nu - ((VIO_Real) D-1.0) );
    p = t1 * t2 * t3;

    print( "%g\n", p );

    return( 0 );
}

static  VIO_Real  gamma_func(
    VIO_Real   x )
{
    VIO_Real   lg, g;

    lg = gamma(x);
    g = (VIO_Real) signgam * exp( lg );

    return( g );
}
