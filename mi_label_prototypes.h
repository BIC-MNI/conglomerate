#ifndef  DEF_mi_label_prototypes
#define  DEF_mi_label_prototypes

  VIO_BOOL  get_label_lookup_var(
    int   minc_id,
    int   *label_var );

  VIO_BOOL  read_label_lookup(
    int      minc_id,
    int      *n_labels,
    int      *values[],
    VIO_STR   *labels[] );

  VIO_BOOL  write_label_lookup(
    int      minc_id,
    int      n_labels,
    int      values[],
    VIO_STR   labels[] );

  void  add_label_to_list(
    int      *n_labels,
    int      *values[],
    VIO_STR   *labels[],
    int      value_to_add,
    VIO_STR   label_to_add );

  VIO_BOOL  delete_label_from_list(
    int      *n_labels,
    int      *values[],
    VIO_STR   *labels[],
    int      value_to_delete );

  VIO_BOOL  lookup_value_for_label(
    int      n_labels,
    int      values[],
    VIO_STR   labels[],
    VIO_STR   label,
    int      *value );

  VIO_BOOL  lookup_label_for_value(
    int      n_labels,
    int      values[],
    VIO_STR   labels[],
    int      value,
    VIO_STR   *label );

  void  print_label(
    int      value,
    VIO_STR   label );
#endif
