#ifndef  DEF_CONJUGATE_MIN_PROTOTYPES
#define  DEF_CONJUGATE_MIN_PROTOTYPES

public   conjugate_min  conjugate_min_initialize(
    int                    n_parameters,
    Real                   initial[],
    Real                   (*function) ( Real [], void * ),
    void                   (*deriv_function) ( Real [], void *, Real [] ),
    void                   *function_data,
    Real                   line_min_range_tolerance,
    Real                   line_min_domain_tolerance,
    int                    max_iterations,
    int                    max_restarts,
    Real                   *current_value );

public  BOOLEAN  conjugate_min_do_one_iteration(  
    conjugate_min     conj,
    Real              *resulting_value );

public  void  conjugate_min_get_current_position(  
    conjugate_min     conj,
    Real              parameters[] );

public  int  conjugate_min_get_n_iterations(  
    conjugate_min     conj );

public  void  conjugate_min_print_iteration_info(  
    conjugate_min     conj );

public  void  conjugate_min_terminate(  
    conjugate_min     conj );
#endif
