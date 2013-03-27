#ifndef  DEF_CONJUGATE_MIN_PROTOTYPES
#define  DEF_CONJUGATE_MIN_PROTOTYPES

public   conjugate_min  conjugate_min_initialize(
    int                    n_parameters,
    VIO_Real                   initial[],
    VIO_Real                   (*function) ( VIO_Real [], void * ),
    void                   (*deriv_function) ( VIO_Real [], void *, VIO_Real [] ),
    void                   *function_data,
    VIO_Real                   termination_range_tolerance,
    VIO_Real                   termination_domain_tolerance,
    VIO_Real                   line_min_range_tolerance,
    VIO_Real                   line_min_domain_tolerance,
    int                    max_iterations,
    int                    max_restarts,
    VIO_Real                   *current_value );

public  VIO_BOOL  conjugate_min_do_one_iteration(  
    conjugate_min     conj,
    VIO_Real              *resulting_value );

public  void  conjugate_min_get_current_position(  
    conjugate_min     conj,
    VIO_Real              parameters[] );

public  int  conjugate_min_get_n_iterations(  
    conjugate_min     conj );

public  void  conjugate_min_print_iteration_info(  
    conjugate_min     conj );

public  void  conjugate_min_terminate(  
    conjugate_min     conj );

public  VIO_Real  conjugate_minimize_function(
    int                    n_parameters,
    VIO_Real                   initial[],
    VIO_Real                   (*function) ( VIO_Real [], void * ),
    void                   (*deriv_function) ( VIO_Real [], void *, VIO_Real [] ),
    void                   *function_data,
    VIO_Real                   termination_range_tolerance,
    VIO_Real                   termination_domain_tolerance,
    VIO_Real                   line_min_range_tolerance,
    VIO_Real                   line_min_domain_tolerance,
    int                    max_iterations,
    int                    max_restarts,
    VIO_Real                   solution[] );
#endif
