#ifndef  DEF_LINE_MINIMIZATION_PROTOTYPES
#define  DEF_LINE_MINIMIZATION_PROTOTYPES

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
    Real     *max_movement );
#endif
