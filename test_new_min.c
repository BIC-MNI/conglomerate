#include  <bicpl.h>
#include  <volume_io.h>

#undef  FAST
#define  FAST

#define  N_PARAMETERS   2

static int count_f = 0;
static int count_fd = 0;

private  VIO_Real  function(
    VIO_Real   parameters[],
    void   *function_data )
{
    VIO_Real   x1, x2;
++count_f;

    x1 = parameters[0];
    x2 = parameters[1];
    return( 100.0 * (x2 - x1 * x1) * (x2 - x1 * x1) +
            (1.0 - x1) * (1.0 - x1) );
}

private  void  function_deriv(
    VIO_Real   parameters[],
    void   *function_data,
    VIO_Real   deriv[] )
{
    VIO_Real   x1, x2;

++count_fd;

    x1 = parameters[0];
    x2 = parameters[1];

    deriv[0] = 200.0 * (x2 - x1 * x1) * (-2.0) * x1 + 2.0 * (1.0 - x1) * (-1.0);
    deriv[1] = 200.0 * (x2 - x1 * x1);
}

#define  SEARCH_RATIO           5.0
#define  GOLDEN_RATIO   0.618034
#define  DEFAULT_STEP           1.0e-10

private  VIO_Real  evaluate_along_line(
    int      n_parameters,
    VIO_Real     parameters[],
    VIO_Real     line_direction[],
    VIO_Real     t,
    VIO_Real     test_parameters[],
    VIO_Real     (*function) ( VIO_Real [], void * ),
    void     *function_data )
{
    int   parm;

    for_less( parm, 0, n_parameters )
        test_parameters[parm] = parameters[parm] + t * line_direction[parm];

    return( (*function) ( test_parameters, function_data ) );
}

private VIO_BOOL  minimize_along_line(
    int      n_parameters,
    VIO_Real     parameters[],
    VIO_Real     line_direction[],
    VIO_Real     test_parameters[],
    VIO_Real     (*function) ( VIO_Real [], void * ),
    void     *function_data,
    VIO_Real     range_tolerance,
    VIO_Real     domain_tolerance,
    VIO_Real     current_value,
    VIO_Real     *final_value )
{
    int              p;
    VIO_Real             t0, t1, t2, f0, f1, f2, t_next, f_next, step, fsize;
    VIO_Real             swap, bottom, new_size, old_size;
    VIO_Real             search_ratio;
    VIO_BOOL          done, prev_failed;

    search_ratio = SEARCH_RATIO;

    t0 = 0.0;
    f0 = current_value;

    if( domain_tolerance <= 0.0 )
        step = DEFAULT_STEP;
    else
        step = domain_tolerance;

    do
    {
        t1 = step;
        f1 = evaluate_along_line( n_parameters, parameters, line_direction,
                                  t1, test_parameters,
                                  function, function_data );

        step *= search_ratio;
    }
    while( f1 == f0 );

    step /= 2.0;

    if( f1 > f0 )
    {
        swap = t1;
        t1 = t0;
        t0 = swap;

        swap = f1;
        f1 = f0;
        f0 = swap;

        step = -step;
    }

    done = FALSE;
    while( !done )
    {
        t2 = t1 + step;
        step *= search_ratio;

        f2 = evaluate_along_line( n_parameters, parameters, line_direction,
                                  t2, test_parameters,
                                  function, function_data );

        if( f2 > f1 )
            done = TRUE;
        else
        {
            t0 = t1;
            f0 = f1;
            t1 = t2;
            f1 = f2;
        }
    }

    if( t2 < t0 )
    {
        swap = t2;
        t2 = t0;
        t0 = swap;

        swap = f2;
        f2 = f0;
        f0 = swap;
    }

    prev_failed = FALSE;

    do
    {
        bottom = 2.0 * (t2*f0-t0*f2-t1*f0+t1*f2-t2*f1+t0*f1);

        if( bottom != 0.0 )
        {
            t_next = (-t0*t0*f2+t0*t0*f1+t1*t1*f2-f0*t1*t1+f0*t2*t2-f1*t2*t2)/
                     bottom;
        }

        if( prev_failed || bottom == 0.0 ||
            t_next <= t0 ||
            t_next >= t2 ||
            t_next == t1 )
        {
            if( t1 - t0 > t2 - t1 )
                t_next = t0 + (t1 - t0) * GOLDEN_RATIO;
            else
                t_next = t2 + (t1 - t2) * GOLDEN_RATIO;
        }

        f_next = evaluate_along_line( n_parameters, parameters, line_direction,
                                      t_next, test_parameters,
                                      function, function_data );

        old_size = t2 - t0;

        if( f_next < f1 && t_next < t1 ||
            f_next == f1 && t_next < t1 && t1 - t0 < t2 - t_next )
        {
            t2 = t1;
            f2 = f1;
            t1 = t_next;
            f1 = f_next;
        }
        else if( f_next < f1 && t_next >= t1 ||
                 f_next == f1 && t_next >= t1 && t2 - t1 < t_next - t0 )
        {
            t0 = t1;
            f0 = f1;
            t1 = t_next;
            f1 = f_next;
        }
        else if( f_next > f1 && t_next < t1 ||
                 f_next == f1 && t_next < t1 && t1 - t0 >= t2 - t_next )
        {
            t0 = t_next;
            f0 = f_next;
        }
        else if( f_next > f1 && t_next >= t1 ||
                 f_next == f1 && t_next >= t1 && t2 - t1 >= t_next - t0 )
        {
            t2 = t_next;
            f2 = f_next;
        }

        new_size = t2 - t0;
        if( (new_size / old_size) > .99 )
            prev_failed = TRUE;
        else
            prev_failed = FALSE;

        fsize = MAX( f0, f2 ) - f1;
    }
    while( 
#ifdef FAST
           f1 >= current_value && 
#endif
           (domain_tolerance < 0.0 || (t2 - t0) > domain_tolerance) &&
           (range_tolerance < 0.0 || fsize > range_tolerance) );

    if( t1 != 0.0 )
    {
        *final_value = f1;
        for_less( p, 0, n_parameters )
            parameters[p] += t1 * line_direction[p];
    }
    else
        *final_value = current_value;

    return( f1 != current_value );
}

public  VIO_Real  new_minimize(
    int                    n_parameters,
    VIO_Real                   initial[],
    VIO_Real                   (*function) ( VIO_Real [], void * ),
    void                   (*deriv_function) ( VIO_Real [], void *, VIO_Real [] ),
    void                   *function_data,
    VIO_Real                   line_min_range_tolerance,
    VIO_Real                   line_min_domain_tolerance,
    int                    max_iterations,
    int                    max_restarts,
    VIO_Real                   solution[] )
{
    int             parm, n_no_shrink, iter;
    VIO_Real            value, *derivative, step, len;
    VIO_Real            *test_pos, *test_parameters, *derivative_at_test;
    VIO_Real            current_value, new_value;
    VIO_BOOL         success, shrunk;

    ALLOC( derivative, n_parameters );
    ALLOC( test_pos, n_parameters );
    ALLOC( test_parameters, n_parameters );
    ALLOC( derivative_at_test, n_parameters );

    for_less( parm, 0, n_parameters )
        solution[parm] = initial[parm];

    step = 1e-4;
    n_no_shrink = 0;

    current_value = (*function) ( solution, function_data );

    for_less( iter, 0, max_iterations )
    {
        (*deriv_function) ( solution, function_data, derivative );

        len = 0.0;
        for_less( parm, 0, n_parameters )
            len += derivative[parm] * derivative[parm];

        if( len == 0.0 )
            break;

        len = sqrt( len );
        for_less( parm, 0, n_parameters )
            derivative[parm] /= len;

        shrunk = FALSE;

        while( step > 0.0 )
        {
            for_less( parm, 0, n_parameters )
                test_pos[parm] = solution[parm] - step * derivative[parm];

            (*deriv_function) ( test_pos, function_data, derivative_at_test );

            len = 0.0;
            for_less( parm, 0, n_parameters )
                len += derivative_at_test[parm] * derivative_at_test[parm];

            if( len != 0.0 )
            {
                len = sqrt( len );
                for_less( parm, 0, n_parameters )
                    derivative_at_test[parm] /= len;

                success = minimize_along_line( n_parameters,
                                               test_pos,
                                               derivative_at_test,
                                               test_parameters,
                                               function,
                                               function_data,
                                               line_min_range_tolerance,
                                               line_min_domain_tolerance,
                                               current_value,
                                               &new_value );
            }
            else
                success = FALSE;

            if( success )
                break;

            shrunk = TRUE;
            step /= 2.0;
        }

        if( !success )
            break;

        if( shrunk )
            n_no_shrink = 0;
        else
        {
            ++n_no_shrink;
            if( n_no_shrink >= 1 )
            {
                n_no_shrink = 0;
                step *= 2.0;
            }
        }

        for_less( parm, 0, n_parameters )
            solution[parm] = test_parameters[parm];
        current_value = new_value;

if( iter % 1000 == 0 )
        print( "Iter: %d  Pos: %g %g  Fit: %g    Step: %g\n", iter+1,
               solution[0], solution[1], current_value,
               step );
    }

    FREE( derivative );
    FREE( test_pos );
    FREE( test_parameters );
    FREE( derivative_at_test );

    return( current_value );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Real            initial[N_PARAMETERS], value;
    VIO_Real            range_tolerance, domain_tolerance;
    int             n_iterations;

    initialize_argument_processing( argc, argv );

    (void) get_real_argument( -1.0, &initial[0] );
    (void) get_real_argument( 1.2, &initial[1] );
    (void) get_real_argument( 0.01, &range_tolerance );
    (void) get_real_argument( 0.01, &domain_tolerance );
    (void) get_int_argument( 100, &n_iterations );

    value = new_minimize( 2, initial, function, function_deriv,
                          NULL, range_tolerance,
                          domain_tolerance, n_iterations, 2,
                          initial );

    print( "\n" );
    print( "Solution is at %g %g: %g\n", initial[0], initial[1], value );
    print( "Counts: %d %d\n", count_f, count_fd );

    return( 0 );
}
