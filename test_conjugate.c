#include  <bicpl.h>
#include  <internal_volume_io.h>
#include  <conjugate_min.h>

#define  N_PARAMETERS   2

static int count_f = 0;
static int count_fd = 0;

private  Real  function(
    Real   parameters[],
    void   *function_data )
{
    Real   x1, x2;

    ++count_f;

    x1 = parameters[0];
    x2 = parameters[1];
    return( 100.0 * (x2 - x1 * x1) * (x2 - x1 * x1) +
            (1.0 - x1) * (1.0 - x1) );
}

private  void  function_deriv(
    Real   parameters[],
    void   *function_data,
    Real   deriv[] )
{
    Real   x1, x2;

    ++count_fd;

    x1 = parameters[0];
    x2 = parameters[1];

    deriv[0] = 200.0 * (x2 - x1 * x1) * (-2.0) * x1 + 2.0 * (1.0 - x1) * (-1.0);
    deriv[1] = 200.0 * (x2 - x1 * x1);
}

int  main(
    int   argc,
    char  *argv[] )
{
    Real            initial[N_PARAMETERS], value;
    Real            range_tolerance, domain_tolerance;
    Real            line_min_range_tolerance, line_min_domain_tolerance;
    int             n_iterations;

    initialize_argument_processing( argc, argv );

    (void) get_real_argument( -1.0, &initial[0] );
    (void) get_real_argument( 1.2, &initial[1] );
    (void) get_real_argument( 0.01, &range_tolerance );
    (void) get_real_argument( 0.01, &domain_tolerance );
    (void) get_real_argument( 0.01, &line_min_range_tolerance );
    (void) get_real_argument( 0.01, &line_min_domain_tolerance );
    (void) get_int_argument( 100, &n_iterations );

    value = conjugate_minimize_function( 2, initial, function, function_deriv,
                                         NULL,
                                         range_tolerance,
                                         domain_tolerance,
                                         line_min_range_tolerance,
                                         line_min_domain_tolerance,
                                         n_iterations, 2, initial );

    print( "\n" );
    print( "Solution is at %g %g: %g\n", initial[0], initial[1], value );
    print( "Count: %d %d\n", count_f, count_fd );

    return( 0 );
}
