#ifndef  DEF_CONJUGATE_MIN_H
#define  DEF_CONJUGATE_MIN_H

struct  conjugate_min_struct;

typedef  struct  conjugate_min_struct  *conjugate_min;


#ifndef  public
#define       public   extern
#define       public_was_defined_here
#endif

#include  <conjugate_min_prototypes.h>

#ifdef  public_was_defined_here
#undef       public
#undef       public_was_defined_here
#endif


#endif
