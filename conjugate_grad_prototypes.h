#ifndef  DEF_CONJUGATE_GRAD_PROTOTYPES
#define  DEF_CONJUGATE_GRAD_PROTOTYPES

public  void   reinitialize_conjugate_gradient(
    conjugate_grad   con );

public  conjugate_grad   initialize_conjugate_gradient(
    int       n_parameters );

public  void   delete_conjugate_gradient(
    conjugate_grad   con );

public  BOOLEAN  get_conjugate_unit_direction(
    conjugate_grad   con,
    Real        derivative[],
    Real        unit_dir[] );
#endif
