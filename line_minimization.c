#include  <internal_volume_io.h>


#define  SEARCH_RATIO   5.0
#define  GOLDEN_RATIO   0.618034
#define  DEFAULT_STEP   1.0e-10

private  Real  evaluate_along_line(
    int      n_parameters,
    Real     parameters[],
    Real     line_direction[],
    Real     t,
    Real     test_parameters[],
    Real     (*function) ( Real [], void * ),
    void     *function_data )
{
    int   parm;

    for_less( parm, 0, n_parameters )
        test_parameters[parm] = parameters[parm] + t * line_direction[parm];

    return( (*function) ( test_parameters, function_data ) );
}

public  Real  minimize_along_line(
    int      n_parameters,
    Real     parameters[],
    Real     line_direction[],
    Real     test_parameters[],
    Real     (*function) ( Real [], void * ),
    void     *function_data,
    Real     range_tolerance,
    Real     domain_tolerance,
    Real     current_value,
    Real     *max_movement )
{
    int              p;
    Real             t0, t1, t2, f0, f1, f2, t_next, f_next, step, fsize;
    Real             swap, bottom, new_size, old_size;
    Real             search_ratio, prev_step, final_value, movement;
    BOOLEAN          done, prev_failed;

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
    prev_step = -step;
    while( !done && step != prev_step )
    {
        t2 = t1 + step;
        prev_step = step;
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
    while( (domain_tolerance < 0.0 || (t2 - t0) > domain_tolerance) &&
           (range_tolerance < 0.0 || fsize > range_tolerance) );

    *max_movement = 0.0;

    if( t1 != 0.0 && f1 != current_value )
    {
        final_value = f1;
        for_less( p, 0, n_parameters )
        {
            movement = FABS( t1 * line_direction[p] );
            if( movement > *max_movement )
                *max_movement = movement;
            parameters[p] += t1 * line_direction[p];
        }
    }
    else
        final_value = current_value;

    return( final_value );
}
