#ifndef  DEF_mi_label_prototypes
#define  DEF_mi_label_prototypes

public  BOOLEAN  get_label_lookup_var(
    int   minc_id,
    int   *label_var );

public  BOOLEAN  read_label_lookup(
    int      minc_id,
    int      *n_labels,
    int      *values[],
    STRING   *labels[] );

public  BOOLEAN  write_label_lookup(
    int      minc_id,
    int      n_labels,
    int      values[],
    STRING   labels[] );

public  void  add_label_to_list(
    int      *n_labels,
    int      *values[],
    STRING   *labels[],
    int      value_to_add,
    STRING   label_to_add );

public  BOOLEAN  delete_label_from_list(
    int      *n_labels,
    int      *values[],
    STRING   *labels[],
    int      value_to_delete );

public  BOOLEAN  lookup_value_for_label(
    int      n_labels,
    int      values[],
    STRING   labels[],
    STRING   label,
    int      *value );

public  BOOLEAN  lookup_label_for_value(
    int      n_labels,
    int      values[],
    STRING   labels[],
    int      value,
    STRING   *label );

public  void  print_label(
    int      value,
    STRING   label );
#endif
