#ifndef  DEF_CONJUGATE_GRAD_H
#define  DEF_CONJUGATE_GRAD_H

struct  conjugate_grad_struct;

typedef  struct  conjugate_grad_struct  *conjugate_grad;


#ifndef  public
#define       public   extern
#define       public_was_defined_here
#endif

#include  <conjugate_grad_prototypes.h>

#ifdef  public_was_defined_here
#undef       public
#undef       public_was_defined_here
#endif


#endif
