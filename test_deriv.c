#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>
#include  <interval.h>
#include  <ieeefp.h>

static Real  COEF2 = -1.7234e10;
static Real  COEF1 = 2.23132e10;

private  Real  function(
    Real x )
{
    return( x * x * x + COEF2 * x * x + COEF1 * x );
}

private  Real  function_deriv(
    Real x )
{
    return( 3.0 * x * x + 2.0 * COEF2 * x + COEF1 );
}


private  Interval  interval_function(
    Interval x )
{
    Interval  xx, xxx, res, fx, fxx;

    SQUARE_INTERVAL( xx, x );
    MULT_INTERVALS( xxx, xx, x );

    MULT_INTERVAL_REAL( fxx, xx, COEF2 );
    MULT_INTERVAL_REAL( fx, x, COEF1 );

    ADD_INTERVALS( res, xxx, fxx );
    ADD_INTERVALS( res, res, fx );

    return( res );
}

private  Real  interval_get_deriv(
    Real  x )
{
    Interval   x_int, step_int, x2, x1, f1_int, f2_int, top, bottom, deriv_int;
    Real       step, deriv, size;
    Real       tolerance = 1.0e-8;

    SET_INTERVAL( x_int, x, x );
    step = 1.0e-100;
    SET_INTERVAL( step_int, step, step );

    do
    {
        ADD_INTERVALS( x2, x_int, step_int );
        SUBTRACT_INTERVALS( x1, x_int, step_int );
        f1_int = interval_function( x1 );
        f2_int = interval_function( x2 );

        SUBTRACT_INTERVALS( top, f2_int, f1_int );
        SUBTRACT_INTERVALS( bottom, x2, x1 );
        DIVIDE_INTERVALS( deriv_int, top, bottom );

        deriv = INTERVAL_MIDPOINT( deriv_int );
        size = INTERVAL_SIZE( deriv_int );

/*
        print( "%g: %.20g %.20g     %g\n", INTERVAL_MIDPOINT(step_int),
               INTERVAL_MIN(deriv_int),
               INTERVAL_MAX(deriv_int), INTERVAL_SIZE(deriv_int) );
*/

        MULT_INTERVAL_REAL( step_int, step_int, 2.0 );
    }
    while( finite(INTERVAL_MIN(step_int)) &&
           (!finite(INTERVAL_MIN(deriv_int)) ||
            !finite(INTERVAL_MAX(deriv_int)) ||
            isnand(deriv) ||
            (deriv == 0.0 && size > tolerance) ||
            (deriv != 0.0 && size / fabs(deriv) > tolerance)) );

    return( deriv );
}

int  main(
    int  argc,
    char *argv[] )
{
    int        i, start, end, n_steps;
    Real       x, ln_coef1, ln_coef2;
    Real       ln_min, ln_max, truth, test;
    Real       range;

    initialize_argument_processing( argc, argv );

    (void) get_int_argument( 100, &n_steps );
    (void) get_int_argument( 0, &start );
    (void) get_int_argument( n_steps, &end );

    ln_min = log( 1.0e-30 );
    ln_max = log( 1.0e30 );

    set_random_seed( 1234331121 );

    for_less( i, 0, n_steps )
    {
        x = ln_min + (ln_max - ln_min) * get_random_0_to_1();
        ln_coef1 = ln_min + (ln_max - ln_min) * get_random_0_to_1();
        ln_coef2 = ln_min + (ln_max - ln_min) * get_random_0_to_1();

        if( i < start || i > end )
            continue;

        COEF1 = exp( ln_coef1 );
        COEF2 = exp( ln_coef2 );

        truth = function_deriv( x );
        test = interval_get_deriv( x );

        if( !numerically_close( truth, test, 1.0e-8 ) )
        {
            print( "%d: x c1 c2   : %g %g %g\n", i, x, COEF1, COEF2 );
            print( "Deriv     : %.20g\n", truth  );
            print( "Test Deriv: %.20g\n", test );
            print( "\n" );
        }
    }

    return( 0 );
}
