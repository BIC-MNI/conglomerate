#ifndef  DEF_MINC_LABELS
#define  DEF_MINC_LABELS

#include  <bicpl.h>

#ifndef  public
#define       public   extern
#define       public_was_defined_here
#endif

#include  <mi_label_prototypes.h>

#ifdef  public_was_defined_here
#undef       public
#undef       public_was_defined_here
#endif

#endif
