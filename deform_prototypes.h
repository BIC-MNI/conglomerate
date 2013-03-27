#ifndef  DEF_deform_prototypes
#define  DEF_deform_prototypes

public  void  deform_lines(
    lines_struct      *lines,
    deform_struct     *deform_parms );

public  void  deform_lines_one_iteration(
    lines_struct      *lines,
    deform_struct     *deform_parms,
    int               iteration );

public  void  get_line_equilibrium_point(
    lines_struct                 *lines,
    int                          axis,
    int                          point_index,
    int                          neighbours[],
    VIO_Real                         curvature_factors[],
    VIO_Real                         max_search_distance,
    int                          degrees_continuity,
    VIO_Volume                       volume,
    VIO_Volume                       label_volume,
    boundary_definition_struct   *boundary_def,
    deformation_model_struct     *deformation_model,
    VIO_Point                        *equilibrium_point,
    VIO_Point                        *boundary_point );

public  int  find_axial_plane(
    lines_struct   *lines );

public  void  deform_polygons(
    polygons_struct   *polygons,
    deform_struct     *deform_parms );

public  void  deform_polygons_one_iteration(
    polygons_struct   *polygons,
    deform_struct     *deform_parms,
    int               iteration );

public  VIO_BOOL  find_boundary_in_direction(
    VIO_Volume                      volume,
    VIO_Volume                      label_volume,
    voxel_coef_struct           *lookup,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    VIO_Real                        model_dist,
    VIO_Point                       *ray_origin,
    VIO_Vector                      *unit_pos_dir,
    VIO_Vector                      *unit_neg_dir,
    VIO_Real                        max_outwards_search_distance,
    VIO_Real                        max_inwards_search_distance,
    int                         degrees_continuity,
    boundary_definition_struct  *boundary_def,
    VIO_Real                        *boundary_distance );

public  int  find_voxel_line_polynomial(
    VIO_Real        coefs[],
    int         degrees_continuity,
    int         x,
    int         y,
    int         z,
    VIO_Real        line_origin[],
    VIO_Real        line_direction[],
    VIO_Real        line_poly[] );

public  int  find_voxel_line_value_intersection(
    VIO_Real        coefs[],
    int         degrees_continuity,
    int         x,
    int         y,
    int         z,
    VIO_Real        line_origin[],
    VIO_Real        line_direction[],
    VIO_Real        t_min,
    VIO_Real        t_max,
    VIO_Real        isovalue,
    VIO_Real        distances[3] );

public  void  initialize_deformation_model(
    deformation_model_struct  *model );

public  void  print_deformation_model(
    deformation_model_struct   *deformation_model );

public  VIO_Status   add_deformation_model(
    deformation_model_struct   *deformation_model,
    int                        up_to_n_points,
    VIO_Real                       model_weight,
    char                       model_filename[],
    VIO_Real                       min_curvature_offset,
    VIO_Real                       max_curvature_offset );

public  void  delete_deformation_model(
    deformation_model_struct  *model );

public  VIO_Status  input_original_positions(
    deformation_model_struct  *deform_model,
    char                      position_filename[],
    VIO_Real                      max_position_offset,
    int                       n_deforming_points );

public  VIO_BOOL  check_correct_deformation_polygons(
    polygons_struct            *polygons,
    deformation_model_struct   *model );

public  VIO_BOOL  check_correct_deformation_lines(
    lines_struct               *lines,
    deformation_model_struct   *model );

public  deform_model_struct  *find_relevent_model(
    deformation_model_struct  *model,
    int                       point_index );

public  void  get_model_shape_point(
    VIO_Point    *origin,
    VIO_Vector   *pos_model_dir,
    VIO_Vector   *neg_model_dir,
    VIO_Real     dist,
    VIO_Point    *point );

public  void  compute_equilibrium_point(
    int                       point_index,
    VIO_BOOL                   boundary_exists,
    VIO_Real                      boundary_dist,
    VIO_Real                      base_length,
    VIO_Real                      model_dist,
    VIO_Vector                    *pos_model_dir,
    VIO_Vector                    *neg_model_dir,
    VIO_Point                     *centroid,
    deformation_model_struct  *deformation_model,
    VIO_Point                     *equilibrium_point );

public  void  compute_model_dirs(
    VIO_Point      *centroid,
    VIO_Vector     *normal,
    VIO_Real       base_length,
    VIO_Point      *model_point,
    VIO_Real       *model_dist,
    VIO_Point      *search_origin,
    VIO_Vector     *pos_model_dir,
    VIO_Vector     *neg_model_dir );

public  void  get_model_point(
    deformation_model_struct  *deformation_model,
    VIO_Point                     points[],
    int                       point_index,
    int                       n_neighbours,
    int                       neighbours[],
    VIO_Real                      curvatures[],
    VIO_Point                     *centroid,
    VIO_Vector                    *normal,
    VIO_Real                      base_length,
    VIO_Point                     *model_point );

public  void  get_neighbours_of_line_vertex(
    lines_struct    *lines,
    int             vertex_index,
    int             neighbours[2] );

public  VIO_BOOL  deformation_model_includes_average(
    deformation_model_struct   *model );

public  VIO_Real  compute_line_curvature(
    lines_struct    *lines,
    int             axis,
    int             point_index,
    int             prev_point_index,
    int             next_point_index );

public  VIO_Real  deform_point(
    int                        point_index,
    VIO_Point                      points[],
    VIO_Point                      *equilibrium_point,
    VIO_Real                       fractional_step,
    VIO_Real                       max_step,
    VIO_BOOL                    position_constrained,
    VIO_Real                       max_position_offset,
    VIO_Point                      original_positions[],
    VIO_Point                      *new_point );

public  void  compute_line_centroid_and_normal(
    lines_struct     *lines,
    int              axis,
    int              prev_point_index,
    int              next_point_index,
    VIO_Point            *centroid,
    VIO_Vector           *normal,
    VIO_Real             *base_length );

public  int  get_subsampled_neighbours_of_point(
    deformation_model_struct  *deformation_model,
    polygons_struct           *polygons,
    int                       poly,
    int                       vertex_index,
    int                       neighbours[],
    int                       max_neighbours,
    VIO_BOOL                   *interior_flag );

public  VIO_BOOL  is_point_inside_surface(
    VIO_Volume                      volume,
    VIO_Volume                      label_volume,
    int                         continuity,
    VIO_Real                        voxel[],
    VIO_Vector                      *direction,
    boundary_definition_struct  *boundary_def );

public  void   get_centre_of_cube(
    VIO_Point       *cube,
    int         sizes[3],
    VIO_Point       *centre );

public  VIO_BOOL  contains_value(
    VIO_Real  values[2][2][2],
    int   sizes[3] );

public  VIO_BOOL  cube_is_small_enough(
    VIO_Point     cube[2],
    int       sizes[3],
    VIO_Real      min_cube_size );

public  void  initialize_deform_stats(
    deform_stats  *stats );

public  void  record_error_in_deform_stats(
    deform_stats   *stats,
    VIO_Real           error );

public  void  print_deform_stats(
    deform_stats   *stats,
    int            n_points );

public  VIO_BOOL   get_max_point_cube_distance(
    VIO_Point   cube[2],
    int     sizes[3],
    VIO_Point   *point,
    VIO_Real    *distance );

public  void  initialize_deformation_parameters(
    deform_struct  *deform );

public  void  delete_deformation_parameters(
    deform_struct  *deform );

public  void  set_boundary_definition(
    boundary_definition_struct  *boundary_def,
    VIO_Real                        min_value,
    VIO_Real                        max_value,
    VIO_Real                        grad_threshold,
    VIO_Real                        angle,
    char                        direction,
    VIO_Real                        tolerance );

public  void  initialize_lookup_volume_coeficients(
    voxel_coef_struct  *lookup );

public  void  lookup_volume_coeficients(
    voxel_coef_struct  *lookup,
    VIO_Volume             volume,
    int                degrees_continuity,
    int                x,
    int                y,
    int                z,
    VIO_Real               c[] );

public  void  delete_lookup_volume_coeficients(
    voxel_coef_struct  *lookup );
#endif
