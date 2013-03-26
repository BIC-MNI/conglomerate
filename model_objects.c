#include  <volume_io.h>
#include  <deform.h>
#include  <limits.h>

static  void  set_deformation_model(
    deformation_model_struct  *model,
    int                       up_to_n_points,
    VIO_Real                      model_weight,
    Deformation_model_types   model_type,
    object_struct             *model_object,
    VIO_Real                      min_curvature_offset,
    VIO_Real                      max_curvature_offset );

  void  initialize_deformation_model(
    deformation_model_struct  *model )
{
    model->n_models = 0;

    set_deformation_model( model, -1, 0.5, FLAT_MODEL,
                           (object_struct *) NULL, -0.3, 0.3 );

    model->position_constrained = FALSE;
}

static  void  delete_model_points(
    deform_model_struct   *model )
{
    if( model->n_model_points > 0 )
    {
        FREE( model->model_centroids );
        FREE( model->model_normals );
        FREE( model->model_points );
        model->n_model_points = 0;
    }
}

static  void  delete_deform_model(
    deform_model_struct   *model )
{
    if( model->model_type == GENERAL_MODEL ||
        model->model_type == PARAMETRIC_MODEL )
    {
        delete_model_points( model );
    }

    if( model->model_object != (object_struct *) NULL )
    {
        delete_object( model->model_object );
        model->model_object = (object_struct *) NULL;
    }
}

static  void  set_deformation_model(
    deformation_model_struct  *model,
    int                       up_to_n_points,
    VIO_Real                      model_weight,
    Deformation_model_types   model_type,
    object_struct             *model_object,
    VIO_Real                      min_curvature_offset,
    VIO_Real                      max_curvature_offset )
{
    int    i, model_index;

    if( up_to_n_points <= 0 )
        up_to_n_points = INT_MAX;

    model_index = 0;

    while( model_index < model->n_models &&
           model->models[model_index].up_to_n_points < up_to_n_points )
        ++model_index;

    if( model_index >= model->n_models ||
        model->models[model_index].up_to_n_points > up_to_n_points )
    {
        SET_ARRAY_SIZE( model->models, model->n_models, model->n_models+1,
                        DEFAULT_CHUNK_SIZE );
        ++model->n_models;

        for( i = model->n_models-1;  i >= model_index + 1;  --i )
            model->models[i] = model->models[i-1];
    }
    else
        delete_deform_model( &model->models[model_index] );

    model->models[model_index].up_to_n_points = up_to_n_points;
    model->models[model_index].model_weight = model_weight;
    model->models[model_index].model_type = model_type;

    if( model_type == GENERAL_MODEL )
        model->models[model_index].model_object = model_object;
    else
        model->models[model_index].model_object = (object_struct *) NULL;

    model->models[model_index].n_model_points = 0;
    model->models[model_index].model_centroids = (VIO_Point *) NULL;
    model->models[model_index].model_normals = (VIO_Vector *) NULL;
    model->models[model_index].model_points = (VIO_Point *) NULL;

    model->models[model_index].min_curvature_offset = min_curvature_offset;
    model->models[model_index].max_curvature_offset = max_curvature_offset;
}

static  void  print_deform_model(
    deform_model_struct   *model )
{
    VIO_STR  model_object_name;
    VIO_STR  type_name;

    switch( model->model_type )
    {
    case FLAT_MODEL:        type_name = "flat";        break;
    case AVERAGE_MODEL:     type_name = "average";     break;
    case PARAMETRIC_MODEL:  type_name = "parametric";  break;
    case GENERAL_MODEL:     type_name = "general";     break;
    default:                type_name = "error";       break;
    }

    if( model->up_to_n_points == INT_MAX )
        print( "All remaining points " );
    else
        print( "Up to %7d points ", model->up_to_n_points );

    print( "%s  Wt: %g ", type_name, model->model_weight );

    if( model->min_curvature_offset <= model->max_curvature_offset )
    {
        print( "   Curv: %g %g ", 
               model->min_curvature_offset, model->max_curvature_offset );
    }

    if( model->model_object != (object_struct *) NULL )
    {
        model_object_name = get_object_name( model->model_object );
        print( "%s ", model_object_name );
        delete_string( model_object_name );
    }

    if( model->n_model_points > 0 )
    {
        print( "# model points: %d", model->n_model_points );
    }

    print( "\n" );
}

  void  print_deformation_model(
    deformation_model_struct   *deformation_model )
{
    int   i;

    for_less( i, 0, deformation_model->n_models )
    {
        print( "Model [%d]: ", i );
        print_deform_model( &deformation_model->models[i] );
    }

    if( deformation_model->position_constrained )
    {
        print( "Position constrained.\n" );
    }
}

  VIO_Status   add_deformation_model(
    deformation_model_struct   *deformation_model,
    int                        up_to_n_points,
    VIO_Real                       model_weight,
    char                       model_filename[],
    VIO_Real                       min_curvature_offset,
    VIO_Real                       max_curvature_offset )
{
    VIO_Status                    status;
    int                       n_objects;
    Deformation_model_types   model_type;
    object_struct             *model_object, **object_list;
    VIO_File_formats              format;

    status = OK;

    model_object = (object_struct *) NULL;

    if( equal_strings( model_filename, "flat" ) )
    {
        model_type = FLAT_MODEL;
    }
    else if( equal_strings( model_filename, "avg" ) )
    {
        model_type = AVERAGE_MODEL;
    }
    else if( equal_strings( model_filename, "parametric" ) )
    {
        model_type = PARAMETRIC_MODEL;
    }
    else
    {
        model_type = GENERAL_MODEL;

        status = input_graphics_file( model_filename, &format,
                                      &n_objects, &object_list );

        if( status == OK && n_objects == 0 )
        {
            print_error( "File %s has no model object.\n", model_filename );
            status = ERROR;
        }
        else
        {
            model_object = object_list[0];
        }
    }

    if( status == OK )
    {
        set_deformation_model( deformation_model, up_to_n_points, model_weight,
                               model_type, model_object,
                               min_curvature_offset, max_curvature_offset );
    }

    return( status );
}

  void  delete_deformation_model(
    deformation_model_struct  *model )
{
    int   i;

    for_less( i, 0, model->n_models )
        delete_deform_model( &model->models[i] );

    FREE( model->models );
    model->n_models = 0;

    if( model->position_constrained )
        FREE( model->original_positions );
}

  VIO_Status  input_original_positions(
    deformation_model_struct  *deform_model,
    char                      position_filename[],
    VIO_Real                      max_position_offset,
    int                       n_deforming_points )
{
    VIO_Status            status, input_status;
    int               i, n_objects, n_points;
    object_struct     **object_list;
    VIO_File_formats      format;
    VIO_Point             *points;
    lines_struct      *lines;
    polygons_struct   *polygons;

    if( deform_model->position_constrained &&
        deform_model->original_positions != (VIO_Point *) NULL )
        FREE( deform_model->original_positions );

    if( equal_strings( position_filename, "none" ) )
    {
        deform_model->position_constrained = FALSE;
    }

    status = input_graphics_file( position_filename, &format,
                                  &n_objects, &object_list );

    input_status = status;

    if( status == OK && n_objects >= 1 && object_list[0]->object_type == LINES )
    {
        lines = get_lines_ptr( object_list[0] );
        n_points = lines->n_points;
        points = lines->points;
    }
    else if( status == OK && n_objects >= 1 &&
             object_list[0]->object_type == POLYGONS )
    {
        polygons = get_polygons_ptr( object_list[0] );
        n_points = polygons->n_points;
        points = polygons->points;
    }
    else
        status = ERROR;

    if( n_points != n_deforming_points )
    {
        print_error( "Incorrect # of points in original positions file.\n" );
        status = ERROR;
    }

    if( status == OK )
    {
        ALLOC( deform_model->original_positions, n_points );
        for_less( i, 0, n_points )
            deform_model->original_positions[i] = points[i];
        deform_model->position_constrained = TRUE;
        deform_model->max_position_offset = max_position_offset;
    }
    else
        deform_model->position_constrained = FALSE;

    if( input_status == OK )
        delete_object_list( n_objects, object_list );

    return( status );
}

static  void  compute_line_model_info(
    lines_struct   *lines,
    VIO_Point          centroids[],
    VIO_Vector         normals[] )
{
    int      size, axis, a1, a2, start_index, end_index, vertex_index;
    int      point_index, neighbours[2], i;
    VIO_Vector   dir_to_next;
    BOOLEAN  closed;

    axis = find_axial_plane( lines );
    a1 = (axis + 1) % 3;
    a2 = (axis + 2) % 3;

    i = 0;
    size = GET_OBJECT_SIZE( *lines, i );

    closed = (size == lines->n_points+1);

    if( closed )
    {
        start_index = 0;
        end_index = lines->n_points-1;
    }
    else
    {
        start_index = 1;
        end_index = lines->n_points-2;
    }

    for_inclusive( vertex_index, start_index, end_index )
    {
        get_neighbours_of_line_vertex( lines, vertex_index, neighbours );

        point_index = lines->indices[vertex_index];

        INTERPOLATE_POINTS( centroids[point_index],
                            lines->points[neighbours[0]],
                            lines->points[neighbours[1]], 0.5 );

        SUB_POINTS( dir_to_next, lines->points[neighbours[1]],
                                 lines->points[neighbours[0]] );

        Point_coord(normals[point_index],axis) = Point_coord(dir_to_next,axis);
        Point_coord(normals[point_index],a1) = Point_coord(dir_to_next,a2);
        Point_coord(normals[point_index],a2) = -Point_coord(dir_to_next,a1);
        NORMALIZE_VECTOR( normals[point_index], normals[point_index] );
    }
}

static  void  compute_polygons_model_info(
    polygons_struct   *polygons,
    VIO_Point             centroids[],
    VIO_Vector            normals[] )
{
    VIO_SCHAR   *points_done;
    int            p, size, poly, vertex_index, point_index;
    VIO_Real           base_length, curvature;

    ALLOC( points_done, polygons->n_points );

    check_polygons_neighbours_computed( polygons );

    for_less( p, 0, polygons->n_points )
        points_done[p] = FALSE;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( vertex_index, 0, size )
        {
            point_index = polygons->indices[
                POINT_INDEX(polygons->end_indices,poly,vertex_index)];

            if( !points_done[point_index] )
            {
                points_done[point_index] = TRUE;

                compute_polygon_point_centroid( polygons, poly,
                              vertex_index, point_index,
                              &centroids[point_index],
                              &normals[point_index], &base_length, &curvature );
            }
        }
    }

    FREE( points_done );
}

static  BOOLEAN  check_correct_general_polygons(
    polygons_struct      *polygons,
    deform_model_struct  *model )
{
    int               i, n_up, n_model_up, n_polygon_points;
    BOOLEAN           sphere_topology, model_sphere_topology;
    BOOLEAN           tetra_topology, model_tetra_topology;
    polygons_struct   *model_polygons, *resized_model_polygons;
    polygons_struct   resized_polygons, half_polygons;

    if( model->model_object == (object_struct *) NULL )
    {
        print_error( "No model object present.\n" );
        return( FALSE );
    }

    if( model->model_object->object_type != POLYGONS )
    {
        print_error( "Model object is not polygons type.\n" );
        return( FALSE );
    }

    model_polygons = get_polygons_ptr( model->model_object );

    sphere_topology = get_tessellation_of_polygons_sphere( polygons, &n_up);
    model_sphere_topology = get_tessellation_of_polygons_sphere(
                                 model_polygons, &n_model_up);

    if( model_sphere_topology != sphere_topology )
    {
        print_error(
          "Model and deforming polygons are not both sphere topology.\n" );
        return( FALSE );
    }

    if( !sphere_topology )
    {
        tetra_topology = is_this_tetrahedral_topology( polygons );
        model_tetra_topology = is_this_tetrahedral_topology( model_polygons );

        if( tetra_topology != model_tetra_topology )
        {
            print_error(
                "Model and deforming polygons are not both tetra topology.\n");
            return( FALSE );
        }
    }
    else
        tetra_topology = FALSE;

    if( model_polygons->n_points != polygons->n_points && !tetra_topology )
    {
        print_error( "Can only subsample tetrahedral topology.\n" );
        return( FALSE );
    }

    n_polygon_points = MIN( polygons->n_points, model->up_to_n_points );
    
    if( model->n_model_points != n_polygon_points )
        delete_model_points( model );

    if( model->n_model_points == 0 )
    {
        if( model_polygons->n_points == n_polygon_points )
            resized_model_polygons = model_polygons;
        else
        {
            resized_polygons = *model_polygons;
            do
            {
                if( sphere_topology )
                {
                    half_sample_sphere_tessellation( &resized_polygons,
                                                     &half_polygons );
                    print_error( "Subdivided sphere tessellation: %d\n",
                           half_polygons.n_points );
                }
                else
                {
                    half_sample_tetrahedral_tessellation(
                                     &resized_polygons,
                                     &half_polygons );
                    print_error( "Subdivided tetrahedal tessellation: %d\n",
                           half_polygons.n_points );
                }

                if( resized_polygons.n_points < n_polygon_points )
                    delete_polygons( &resized_polygons );

                resized_polygons = half_polygons;
            }
            while( resized_polygons.n_points > n_polygon_points );

            if( resized_polygons.n_points != n_polygon_points )
            {
                print_error( "Cannot subsample model polygons.\n" );
                delete_polygons( &resized_polygons );
                return( FALSE );
            }

            resized_model_polygons = &resized_polygons;
        }

        model->n_model_points = n_polygon_points;
        ALLOC( model->model_centroids, n_polygon_points );
        ALLOC( model->model_normals, n_polygon_points );
        ALLOC( model->model_points, n_polygon_points );

        for_less( i, 0, n_polygon_points )
            model->model_points[i] = resized_model_polygons->points[i];

        compute_polygons_model_info( resized_model_polygons,
                                     model->model_centroids,
                                     model->model_normals );

        if( resized_model_polygons != model_polygons )
            delete_polygons( resized_model_polygons );
    }

    return( TRUE );
}

static  BOOLEAN  check_correct_parametric_polygons(
    polygons_struct       *polygons,
    deform_model_struct   *model )
{
    int               n_up, n_model_up, n_polygon_points, n_items;
    VIO_Point             centre;
    BOOLEAN           model_sphere_topology, sphere_topology, correct;
    polygons_struct   *model_polygons;

    n_polygon_points = MIN( model->up_to_n_points, polygons->n_points );

    sphere_topology = get_tessellation_of_polygons_sphere( polygons, &n_up);

    if( model->model_object != (object_struct *) NULL )
    {
        if( model->model_object->object_type != POLYGONS )
            delete_deform_model( model );

        model_polygons = get_polygons_ptr( model->model_object );

        model_sphere_topology = get_tessellation_of_polygons_sphere(
                                     model_polygons, &n_model_up);

        if( sphere_topology != model_sphere_topology ||
            model_polygons->n_points != n_polygon_points )
        {
            delete_deform_model( model );
        }
    }

    if( model->model_object == (object_struct *) NULL )
    {
        model->model_object = create_object( POLYGONS );
        model_polygons = get_polygons_ptr( model->model_object );

        fill_Point( centre, 0.0, 0.0, 0.0 );
        if( sphere_topology )
        {
            n_up = get_tessellation_with_n_points( n_polygon_points );
            print( "Creating parametric sphere: %d\n", n_up );
            create_polygons_sphere( &centre, 1.0, 1.0, 1.0, n_up, 2 * n_up,
                                    FALSE, model_polygons );
        }
        else
        {
            n_items = get_tetra_tessellation_with_n_points( n_polygon_points );
            print( "Creating parametric tetrahedral sphere: %d\n", n_items );
            create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                                       n_items, model_polygons );
        }
    }

    correct = check_correct_general_polygons( polygons, model );

    return( correct );
}

static  BOOLEAN  check_correct_subsampled_polygons(
    polygons_struct       *polygons,
    deform_model_struct   *model )
{
    int               n_up, n_model_up, n_polygon_points, n_items;
    VIO_Point             centre;
    BOOLEAN           model_sphere_topology, sphere_topology;
    polygons_struct   *model_polygons;

    sphere_topology = get_tessellation_of_polygons_sphere( polygons, &n_up);

    if( model->model_object != (object_struct *) NULL )
    {
        if( model->model_object->object_type != POLYGONS )
            delete_deform_model( model );

        model_polygons = get_polygons_ptr( model->model_object );

        model_sphere_topology = get_tessellation_of_polygons_sphere(
                                     model_polygons, &n_model_up);

        if( sphere_topology != model_sphere_topology || 
            (sphere_topology && n_model_up != n_up) )
        {
            delete_deform_model( model );
        }
    }

    if( model->model_object == (object_struct *) NULL )
    {
        n_polygon_points = MIN( model->up_to_n_points, polygons->n_points );

        model->model_object = create_object( POLYGONS );
        model_polygons = get_polygons_ptr( model->model_object );

        fill_Point( centre, 0.0, 0.0, 0.0 );
        if( sphere_topology )
        {
            n_up = get_tessellation_with_n_points( n_polygon_points );
            print( "Creating subsampled sphere: %d\n", n_up );
            create_polygons_sphere( &centre, 1.0, 1.0, 1.0, n_up, 2 * n_up,
                                    FALSE, model_polygons );
        }
        else
        {
            n_items = get_tetra_tessellation_with_n_points( n_polygon_points );
            print( "Creating subsampled tetrahedral sphere: %d\n", n_items );
            create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                                       n_items, model_polygons );
        }
    }

    return( TRUE );
}

static  BOOLEAN  check_correct_model_polygons(
    polygons_struct       *polygons,
    deform_model_struct   *model )
{
    BOOLEAN   correct;

    if( model->model_type == PARAMETRIC_MODEL )
        correct = check_correct_parametric_polygons( polygons, model );
    else if( model->model_type == GENERAL_MODEL )
        correct = check_correct_general_polygons( polygons, model );
    else if( model->up_to_n_points < polygons->n_points )
        correct = check_correct_subsampled_polygons( polygons, model );
    else
        correct = TRUE;

    return( correct );
}

  BOOLEAN  check_correct_deformation_polygons(
    polygons_struct            *polygons,
    deformation_model_struct   *model )
{
    int              model_index;
    BOOLEAN          model_correct;

    model_index = 0;

    model_correct = FALSE;

    while( model_index < model->n_models )
    {
        if( !check_correct_model_polygons( polygons,
                                           &model->models[model_index] ) )
        {
            model_correct = FALSE;
            break;
        }

        model_correct = TRUE;

        if( model->models[model_index].up_to_n_points >= polygons->n_points )
            break;

        ++model_index;
    }

    return( model_correct );
}

static  BOOLEAN  check_correct_general_lines(
    lines_struct         *lines,
    deform_model_struct  *model )
{
    int               i;
    lines_struct      *model_lines;

    if( model->model_object == (object_struct *) NULL )
    {
        print_error( "No model object present.\n" );
        return( FALSE );
    }

    if( model->model_object->object_type != LINES )
    {
        print_error( "Model object is not lines type.\n" );
        return( FALSE );
    }

    model_lines = get_lines_ptr( model->model_object );

    if( model_lines->n_points != lines->n_points )
    {
        print_error(
          "Model lines must have same number of points as deforming lines.\n" );
        return( FALSE );
    }
    
    if( model->n_model_points != lines->n_points )
        delete_model_points( model );

    if( model->n_model_points == 0 )
    {
        model->n_model_points = lines->n_points;
        ALLOC( model->model_centroids, lines->n_points );
        ALLOC( model->model_normals, lines->n_points );
        ALLOC( model->model_points, lines->n_points );

        for_less( i, 0, lines->n_points )
            model->model_points[i] = model_lines->points[i];

        compute_line_model_info( model_lines,
                                 model->model_centroids,
                                 model->model_normals );
    }

    return( TRUE );
}

static  BOOLEAN  check_correct_parametric_lines(
    lines_struct          *lines,
    deform_model_struct   *model )
{
    VIO_Point             centre;
    BOOLEAN           correct;
    lines_struct      *model_lines;

    if( model->model_object != (object_struct *) NULL )
    {
        if( model->model_object->object_type != LINES ||
            model->n_model_points != lines->n_points )
        {
            delete_deform_model( model );
        }
    }

    if( model->model_object == (object_struct *) NULL )
    {
        model->model_object = create_object( LINES );
        model_lines = get_lines_ptr( model->model_object );

        fill_Point( centre, 0.0, 0.0, 0.0 );

        create_line_circle( &centre, Z, 1.0, 1.0, lines->n_points, model_lines);
    }

    correct = check_correct_general_lines( lines, model );

    return( correct );
}

static  BOOLEAN  check_correct_model_lines(
    lines_struct          *lines,
    deform_model_struct   *model )
{
    BOOLEAN   correct;

    if( model->model_type == PARAMETRIC_MODEL )
        correct = check_correct_parametric_lines( lines, model );
    else if( model->model_type == GENERAL_MODEL )
        correct = check_correct_general_lines( lines, model );
    else
        correct = TRUE;

    return( correct );
}

  BOOLEAN  check_correct_deformation_lines(
    lines_struct               *lines,
    deformation_model_struct   *model )
{
    int              model_index;
    BOOLEAN          model_correct;

    model_index = 0;

    model_correct = FALSE;

    while( model_index < model->n_models )
    {
        if( !check_correct_model_lines( lines,
                                        &model->models[model_index] ) )
        {
            model_correct = FALSE;
            break;
        }

        model_correct = TRUE;

        if( model->models[model_index].up_to_n_points >= lines->n_points )
            break;

        ++model_index;
    }

    return( model_correct );
}

  deform_model_struct  *find_relevent_model(
    deformation_model_struct  *model,
    int                       point_index )
{
    int  model_index;

    model_index = 0;
    while( model_index < model->n_models &&
           point_index >= model->models[model_index].up_to_n_points )
    {
        ++model_index;
    }

    if( model_index >= model->n_models )
    {
        HANDLE_INTERNAL_ERROR( "get_model_point" );
    }

    return( &model->models[model_index] );
}
