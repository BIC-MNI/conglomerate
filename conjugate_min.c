#include  <internal_volume_io.h>
#include  <conjugate_grad.h>
#include  <conjugate_min.h>
#include  <line_min_prototypes.h>

#define  DEFAULT_N_RESTARTS  2

struct  conjugate_min_struct
{
    int              n_parameters;
    Real             *parameters;
    Real             *derivative;
    Real             *test_parameters;
    Real             current_value;
    Real             (*function) ( Real [], void * );
    void             (*deriv_function) ( Real [], void *, Real [] );
    void             *function_data;
    Real             line_min_range_tolerance;
    Real             line_min_domain_tolerance;
    Real             termination_range_tolerance;
    Real             termination_domain_tolerance;
    int              n_iterations;
    int              max_iterations;
    int              n_restarts;
    int              max_restarts;

    conjugate_grad   conjugate_dir_info;
};


public   conjugate_min  conjugate_min_initialize(
    int                    n_parameters,
    Real                   initial[],
    Real                   (*function) ( Real [], void * ),
    void                   (*deriv_function) ( Real [], void *, Real [] ),
    void                   *function_data,
    Real                   termination_range_tolerance,
    Real                   termination_domain_tolerance,
    Real                   line_min_range_tolerance,
    Real                   line_min_domain_tolerance,
    int                    max_iterations,
    int                    max_restarts,
    Real                   *current_value )
{
    int               parm;
    conjugate_min     conj;

    ALLOC( conj, 1 );

    conj->conjugate_dir_info = initialize_conjugate_gradient( n_parameters );

    conj->n_parameters = n_parameters;
    if( conj->n_parameters > 0 )
    {
        ALLOC( conj->parameters, n_parameters );
        ALLOC( conj->derivative, n_parameters );
        ALLOC( conj->test_parameters, n_parameters );
    }
    for_less( parm, 0, n_parameters )
        conj->parameters[parm] = initial[parm];
    conj->function = function;
    conj->deriv_function = deriv_function;
    conj->function_data = function_data;
    conj->termination_range_tolerance = termination_range_tolerance;
    conj->termination_domain_tolerance = termination_domain_tolerance;
    conj->line_min_range_tolerance = line_min_range_tolerance;
    conj->line_min_domain_tolerance = line_min_domain_tolerance;
    conj->n_iterations = 0;
    conj->max_iterations = max_iterations;
    conj->n_restarts = 0;
    if( max_restarts < 0 )
        conj->max_restarts = DEFAULT_N_RESTARTS;
    else
        conj->max_restarts = max_restarts;

    conj->current_value = (*conj->function) ( conj->parameters,
                                              conj->function_data );

    if( current_value != NULL )
        *current_value = conj->current_value;

    return( conj );
}

public  BOOLEAN  conjugate_min_do_one_iteration(  
    conjugate_min     conj,
    Real              *resulting_value )
{
    BOOLEAN  success;
    Real     new_value, max_movement;

    if( conj->max_iterations >= 0 && conj->n_iterations >= conj->max_iterations)
        return( FALSE );

    ++conj->n_iterations;

    (*conj->deriv_function) ( conj->parameters, conj->function_data,
                              conj->derivative );

    success = get_conjugate_unit_direction( conj->conjugate_dir_info,
                                            conj->derivative,
                                            conj->derivative );

    if( success )
    {
        new_value = minimize_along_line( conj->n_parameters,
                                       conj->parameters,
                                       conj->derivative,
                                       conj->test_parameters,
                                       conj->function,
                                       conj->function_data,
                                       conj->line_min_range_tolerance,
                                       conj->line_min_domain_tolerance,
                                       conj->current_value,
                                       &max_movement );

        if( conj->current_value - new_value <=
                        conj->termination_range_tolerance &&
            max_movement <= conj->termination_domain_tolerance )
        {
            success = FALSE;
        }

        conj->current_value = new_value;
    }

    if( !success && conj->n_restarts < conj->max_restarts )
    {
        reinitialize_conjugate_gradient( conj->conjugate_dir_info );
        success = TRUE;
        ++conj->n_restarts;
    }

    if( resulting_value != NULL )
        *resulting_value = conj->current_value;

    return( success );
}

public  void  conjugate_min_get_current_position(  
    conjugate_min     conj,
    Real              parameters[] )
{
    int    parm;

    for_less( parm, 0, conj->n_parameters )
        parameters[parm] = conj->parameters[parm];
}

public  int  conjugate_min_get_n_iterations(  
    conjugate_min     conj )
{
    return( conj->n_iterations );
}

public  void  conjugate_min_print_iteration_info(  
    conjugate_min     conj )
{
    print( "Iter  %5d:   Value: %30.16g    N restarts: %2d\n",
           conj->n_iterations, conj->current_value, conj->n_restarts );
}

public  void  conjugate_min_terminate(  
    conjugate_min     conj )
{
    if( conj->n_parameters > 0 )
    {
        FREE( conj->parameters );
        FREE( conj->derivative );
        FREE( conj->test_parameters );
    }

    delete_conjugate_gradient( conj->conjugate_dir_info );
}

public  Real  conjugate_minimize_function(
    int                    n_parameters,
    Real                   initial[],
    Real                   (*function) ( Real [], void * ),
    void                   (*deriv_function) ( Real [], void *, Real [] ),
    void                   *function_data,
    Real                   termination_range_tolerance,
    Real                   termination_domain_tolerance,
    Real                   line_min_range_tolerance,
    Real                   line_min_domain_tolerance,
    int                    max_iterations,
    int                    max_restarts,
    Real                   solution[] )
{
    Real            value;
    conjugate_min   conj;

    conj = conjugate_min_initialize( n_parameters, initial,
                                     function, deriv_function,
                                     function_data,
                                     termination_range_tolerance,
                                     termination_domain_tolerance,
                                     line_min_range_tolerance,
                                     line_min_domain_tolerance,
                                     max_iterations,
                                     max_restarts, &value );

    conjugate_min_print_iteration_info( conj );

    while( conjugate_min_do_one_iteration( conj, &value ) )
    {
        conjugate_min_print_iteration_info( conj );
    }

    conjugate_min_get_current_position( conj, solution );

    conjugate_min_terminate( conj );

    return( value );
}
