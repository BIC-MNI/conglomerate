#ifndef  DEF_LINE_MINIMIZATION_PROTOTYPES
#define  DEF_LINE_MINIMIZATION_PROTOTYPES

VIO_Real  minimize_along_line(
    int      n_parameters,
    VIO_Real     parameters[],
    VIO_Real     line_direction[],
    VIO_Real     test_parameters[],
    VIO_Real     (*function) ( VIO_Real [], void * ),
    void     *function_data,
    VIO_Real     range_tolerance,
    VIO_Real     domain_tolerance,
    VIO_Real     current_value,
    VIO_Real     *max_movement );
#endif
