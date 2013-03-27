#include  <bicpl.h>
#include  <volume_io.h>
#include  <conjugate_min.h>

#define  N_PARAMETERS   2

#ifndef NEW
static  VIO_Real  min_diff = -.02;
static  VIO_Real  max_diff = .02;
static  VIO_Real  d_offset = .1;
static  VIO_Real  init_differential_weight = 1.0e0;
static  VIO_Real  end_differential_weight = 1.0e8;
#else
static  VIO_Real  min_diff = -0.1;
static  VIO_Real  max_diff = 0.1;
static  VIO_Real  d_offset = 0.1;
static  VIO_Real  init_differential_weight = 1.0e3;
static  VIO_Real  end_differential_weight = 1.0e3;
#endif

private  VIO_Real   differential_weights(
    VIO_Real   diff,
    VIO_Real   min_diff,
    VIO_Real   max_diff,
    VIO_Real   diff_offset,
    VIO_Real   ln_start_weight,
    VIO_Real   ln_end_weight )
{
    VIO_Real   fit;

    if( diff < min_diff )
        diff = min_diff - diff;
    else if( diff > max_diff )
        diff = diff - max_diff;
    else
        return( exp(ln_start_weight) * diff * diff );

    if( diff > diff_offset )
        fit = exp(ln_end_weight) * diff * diff;
    else
        fit = exp( ln_start_weight + (ln_end_weight-ln_start_weight) *
                   diff / diff_offset ) * diff * diff;

    return( fit );
}

private  VIO_Real   differential_weights_deriv(
    VIO_Real   diff,
    VIO_Real   min_diff,
    VIO_Real   max_diff,
    VIO_Real   diff_offset,
    VIO_Real   ln_start_weight,
    VIO_Real   ln_end_weight )
{
    VIO_Real   deriv, sign, ln_weight, weight;

    if( diff < min_diff )
    {
        diff = min_diff - diff;
        sign = -1.0;
    }
    else if( diff > max_diff )
    {
        diff = diff - max_diff;
        sign = 1.0;
    }
    else
        return( exp(ln_start_weight) * 2.0 * diff );

    if( diff > diff_offset )
        deriv = sign * exp(ln_end_weight) * 2.0 * diff;
    else
    {
        ln_weight = ln_start_weight + (ln_end_weight -ln_start_weight) * diff/
                    diff_offset;
        weight = exp( ln_weight );
        deriv = weight * sign * 2.0 * diff +
                weight * sign * (ln_end_weight -ln_start_weight) / diff_offset *
                             diff * diff;
    }

    return( deriv );
}

private  VIO_Real  function(
    VIO_Real   parameters[],
    void   *function_data )
{
    VIO_Real   x1, x2, fit, radius_error;

    x1 = parameters[0];
    x2 = parameters[1];

    radius_error = x1 * x1 + x2 * x2 - 1.0;
    fit = x1 * x1 + differential_weights( radius_error, min_diff, max_diff,
                                          d_offset,
                                          log( init_differential_weight ),
                                          log( end_differential_weight ) );
/*
    fit = x1 * x1 + 1.0e4 * radius_error * radius_error;
*/
    return( fit );
}

private  void  function_deriv(
    VIO_Real   parameters[],
    void   *function_data,
    VIO_Real   deriv[] )
{
    VIO_Real   x1, x2, radius_error, d;

    x1 = parameters[0];
    x2 = parameters[1];
    radius_error = x1 * x1 + x2 * x2 - 1.0;
    d = differential_weights_deriv( radius_error, min_diff, max_diff,
                                        d_offset,
                                        log( init_differential_weight ),
                                        log( end_differential_weight ) );

    deriv[0] = 2.0 * x1 + d * 2.0 * x1;
    deriv[1] = d * 2.0 * x2;

/*
    deriv[0] = 2.0 * x1 + 1.0e4 * radius_error * 2.0 * 2.0 * x1;
    deriv[1] = 1.0e4 * radius_error * 2.0 * 2.0 * x2;
*/
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Real            initial[N_PARAMETERS], value;
    VIO_Real            range_tolerance, domain_tolerance;
    VIO_Real            line_min_range_tolerance, line_min_domain_tolerance;
    int             n_iterations;

    initialize_argument_processing( argc, argv );

    (void) get_real_argument( 0.9, &initial[0] );
    (void) get_real_argument( 0.1, &initial[1] );
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

    return( 0 );
}
