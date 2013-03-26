#include  <volume_io.h>
#include  <deform.h>

static  void  perturb_line_points(
    int                          axis,
    lines_struct                 *lines,
    VIO_Real                         curvature_factors[],
    VIO_Point                        new_points[],
    VIO_Real                         fractional_step,
    VIO_Real                         max_step,
    VIO_Real                         max_search_distance,
    int                          degrees_continuity,
    deform_data_struct           *deform_data,
    boundary_definition_struct   *boundary_def,
    deformation_model_struct     *deformation_model,
    deform_stats                 *stats );
static  VIO_Real  one_iteration_lines(
    lines_struct      *lines,
    deform_struct     *deform_parms,
    int               iteration );

  void  deform_lines(
    lines_struct      *lines,
    deform_struct     *deform_parms )
{
    int                iteration;
    VIO_Real               max_error;

    iteration = 0;
    do
    {
        ++iteration;

        max_error = one_iteration_lines( lines, deform_parms, iteration );
    }
    while( max_error > deform_parms->stop_threshold &&
           iteration < deform_parms->max_iterations );
}

  void  deform_lines_one_iteration(
    lines_struct      *lines,
    deform_struct     *deform_parms,
    int               iteration )
{
    (void) one_iteration_lines( lines, deform_parms, iteration );
}

static  VIO_Real  one_iteration_lines(
    lines_struct      *lines,
    deform_struct     *deform_parms,
    int               iteration )
{
    int                axis;
    VIO_Point              *new_points, *tmp;
    VIO_Real               *curvature_factors;
    deform_stats       stats;

    if( lines->n_items > 1 || 
        lines->end_indices[0] != lines->n_points &&
        lines->end_indices[0] != lines->n_points+1 )
    {
        print_error( "Must use on single line.\n" );
        return( 0.0 );
    }

    if( lines->n_points <= 0 )
    {
        print_error( "Must use on nonempty line.\n" );
        return( 0.0 );
    }

    if( !check_correct_deformation_lines( lines,
                                          &deform_parms->deformation_model ) )
    {
        return( 0.0 );
    }

    ALLOC( new_points, lines->n_points );
    ALLOC( curvature_factors, lines->n_points );

    axis = find_axial_plane( lines );

    initialize_deform_stats( &stats );

    perturb_line_points( axis, lines, curvature_factors, new_points,
                         deform_parms->fractional_step,
                         deform_parms->max_step,
                         deform_parms->max_search_distance,
                         deform_parms->degrees_continuity,
                         &deform_parms->deform_data,
                         &deform_parms->boundary_definition,
                         &deform_parms->deformation_model,
                         &stats );

    tmp = lines->points;
    lines->points = new_points;
    new_points = tmp;

    print( "Iteration %d: ", iteration );
    print_deform_stats( &stats, lines->n_points );

    FREE( new_points );
    FREE( curvature_factors );

    return( stats.maximum );
}

  void  get_line_equilibrium_point(
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
    VIO_Point                        *boundary_point )
{
    BOOLEAN          found_flag;
    VIO_Real             base_length, model_distance, boundary_distance;
    VIO_Point            centroid, model_point, search_origin;
    VIO_Vector           normal, pos_model_dir, neg_model_dir;

    compute_line_centroid_and_normal( lines, axis, neighbours[0], neighbours[1],
                                 &centroid, &normal, &base_length );

    get_model_point( deformation_model, lines->points, point_index,
                     2, neighbours, curvature_factors,
                     &centroid, &normal, base_length, &model_point );

    compute_model_dirs( &centroid, &normal, base_length, &model_point,
                        &model_distance, &search_origin,
                        &pos_model_dir, &neg_model_dir );

    found_flag = find_boundary_in_direction( volume, label_volume, NULL,
                                NULL, NULL, model_distance,
                                &search_origin, &pos_model_dir, &neg_model_dir,
                                max_search_distance, max_search_distance,
                                degrees_continuity,
                                boundary_def, &boundary_distance );

    if( boundary_point != (VIO_Point *) NULL )
        get_model_shape_point( &model_point, &pos_model_dir, &neg_model_dir,
                               boundary_distance, boundary_point );

    compute_equilibrium_point( point_index,
                               found_flag, boundary_distance, base_length,
                               model_distance, &pos_model_dir, &neg_model_dir,
                               &centroid, deformation_model,
                               equilibrium_point );
}

static  void  perturb_line_points(
    int                          axis,
    lines_struct                 *lines,
    VIO_Real                         curvature_factors[],
    VIO_Point                        new_points[],
    VIO_Real                         fractional_step,
    VIO_Real                         max_step,
    VIO_Real                         max_search_distance,
    int                          degrees_continuity,
    deform_data_struct           *deform_data,
    boundary_definition_struct   *boundary_def,
    deformation_model_struct     *deformation_model,
    deform_stats                 *stats )
{
    int              point_index, vertex_index;
    int              size, start_index, end_index;
    int              neighbours[2], i;
    VIO_Point            equilibrium_point;
    BOOLEAN          closed;
    VIO_progress_struct  progress;
    VIO_Real             dist_from_equil;

    for_less( point_index, 0, lines->n_points )
        new_points[point_index] = lines->points[point_index];

    i = 0;
    size = GET_OBJECT_SIZE( *lines, i );

    closed = (size == lines->n_points+1);

    if( closed )
    {
        start_index = 0;
        end_index = size-2;
    }
    else
    {
        start_index = 1;
        end_index = size-2;
    }

    if( deformation_model_includes_average(deformation_model) )
    {
        for_inclusive( vertex_index, start_index, end_index )
        {
            get_neighbours_of_line_vertex( lines, vertex_index, neighbours );
            point_index = lines->indices[vertex_index];
            curvature_factors[point_index] = compute_line_curvature(
                    lines, axis, point_index, neighbours[0], neighbours[1] );
        }
    }

    initialize_progress_report( &progress, TRUE, end_index-start_index+1,
                                "Deforming Line Points" );

    for_inclusive( vertex_index, start_index, end_index )
    {
        get_neighbours_of_line_vertex( lines, vertex_index, neighbours );
        point_index = lines->indices[vertex_index];

        get_line_equilibrium_point( lines, axis, point_index, neighbours,
                                    curvature_factors,
                                    max_search_distance, degrees_continuity,
                                    deform_data->volume,
                                    deform_data->label_volume,
                                    boundary_def,
                                    deformation_model, &equilibrium_point,
                                    (VIO_Point *) NULL );

        dist_from_equil = deform_point( point_index, lines->points,
                                        &equilibrium_point,
                                        fractional_step, max_step,
                                        deformation_model->position_constrained,
                                        deformation_model->max_position_offset,
                                        deformation_model->original_positions,
                                        &new_points[point_index] );

        record_error_in_deform_stats( stats, dist_from_equil );

        update_progress_report( &progress, vertex_index - start_index + 1 );
    }

    terminate_progress_report( &progress );
}

  int  find_axial_plane(
    lines_struct   *lines )
{
    int      axis, p;
    BOOLEAN  found_axis;

    found_axis = FALSE;

    for_less( axis, 0, 3 )
    {
        found_axis = TRUE;
        for_less( p, 0, lines->n_points-1 )
        {
            if( Point_coord(lines->points[p],axis) !=
                Point_coord(lines->points[p+1],axis) )
            {
                found_axis = FALSE;
                break;
            }
        }
        if( found_axis ) break;
    }

    if( !found_axis )
    {
        print_error( "No axis found.\n" );
        axis = X;
    }

    return( axis );
}
