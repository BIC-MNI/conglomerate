#include  <volume_io.h>
#include  <bicpl.h>
#include  <deform.h>

#undef   USING_FLOAT
#define  USING_FLOAT

#ifdef USING_FLOAT

typedef  float  ftype;
#define  MINIMIZE_LSQ    minimize_lsq_float
#define  INITIALIZE_LSQ  initialize_lsq_terms_float
#define  ADD_TO_LSQ      add_to_lsq_terms_float
#define  RESET_LSQ       reset_lsq_terms_float
#define  REALLOC_LSQ     realloc_lsq_terms_float
#define  DELETE_LSQ      delete_lsq_terms_float

#else

typedef  VIO_Real   ftype;
#define  MINIMIZE_LSQ    minimize_lsq
#define  MINIMIZE_LSQ    minimize_lsq
#define  INITIALIZE_LSQ  initialize_lsq_terms
#define  ADD_TO_LSQ      add_to_lsq_terms
#define  RESET_LSQ       reset_lsq_terms
#define  REALLOC_LSQ     realloc_lsq_terms
#define  DELETE_LSQ      delete_lsq_terms

#endif

static  void   fit_polygons(
    int                n_points,
    int                n_neighbours[],
    int                *neighbours[],
    VIO_Point              surface_points[],
    VIO_Point              model_points[],
    VIO_Real               model_weight,
    VIO_Real               centroid_weight,
    VIO_Volume             volume,
    VIO_Real               threshold,
    char               normal_direction,
    VIO_Real               tangent_weight,
    VIO_Real               max_outward,
    VIO_Real               max_inward,
    VIO_SCHAR       fit_this_node[],
    VIO_BOOL            floating_flag,
    int                oversample,
    VIO_Real               max_step,
    int                n_iters,
    int                n_iters_recompute );

static  void  usage(
    char   executable_name[] )
{
    VIO_STR  usage_format = "\
Usage:     %s  input.obj output.obj model.obj model_weight centroid_weight \n\
                  volume.mnc threshold +|-|0  tangent_weight out_dist in_dist\n\
                  float/nofloat oversample\n\
                  [n_iters] [n_between] [max_step]\n\
                  [values_file min max]\n\n";

    print_error( usage_format, executable_name );
}

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR               input_filename, output_filename, model_filename;
    VIO_STR               surface_direction, volume_filename, floating_string;
    VIO_STR               values_filename;
    FILE                 *file;
    int                  i, n_objects, n_m_objects;
    int                  n_iters, n_iters_recompute, n_points;
    int                  *n_neighbours, **neighbours, oversample;
    VIO_File_formats         format;
    object_struct        **object_list, **m_object_list;
    polygons_struct      *surface, *model_surface;
    VIO_Volume               volume;
    VIO_Point                *surface_points, *model_points;
    VIO_Real                 threshold, model_weight, centroid_weight;
    VIO_Real                 max_outward, max_inward, tangent_weight;
    VIO_SCHAR         *fit_this_node;
    VIO_Real                 min_value, max_value, value, max_step;
    VIO_BOOL              floating_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &model_filename ) ||
        !get_real_argument( 0.0, &model_weight ) ||
        !get_real_argument( 0.0, &centroid_weight ) ||
        !get_string_argument( NULL, &volume_filename ) ||
        !get_real_argument( 0.0, &threshold ) ||
        !get_string_argument( NULL, &surface_direction ) ||
        !get_real_argument( 0.0, &tangent_weight ) ||
        !get_real_argument( 0.0, &max_outward ) ||
        !get_real_argument( 0.0, &max_inward ) ||
        !get_string_argument( NULL, &floating_string ) ||
        !get_int_argument( 0, &oversample ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_iters );
    (void) get_int_argument( 1, &n_iters_recompute );
    (void) get_real_argument( -1.0, &max_step );
    (void) get_string_argument( NULL, &values_filename );
    (void) get_real_argument( 0.0, &min_value );
    (void) get_real_argument( 0.0, &max_value );

    floating_flag = equal_strings( floating_string, "float" );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    surface = get_polygons_ptr( object_list[0] );
    create_polygon_point_neighbours( surface, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );
    n_points = surface->n_points;
    surface_points = surface->points;
    ALLOC( surface->points, 1 );
    delete_object_list( n_objects, object_list );

    if( input_graphics_file( model_filename, &format, &n_m_objects,
                             &m_object_list ) != VIO_OK || n_m_objects != 1 ||
        get_object_type(m_object_list[0]) != POLYGONS )
        return( 1 );

    model_surface = get_polygons_ptr( m_object_list[0] );
    model_points = model_surface->points;
    ALLOC( model_surface->points, 1 );
    delete_object_list( n_m_objects, m_object_list );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != VIO_OK )
        return( 1 );

    if( values_filename != NULL )
    {
        ALLOC( fit_this_node, n_points );
        if( open_file( values_filename, READ_FILE, ASCII_FORMAT, &file ) != VIO_OK )
            return( 1 );

        for_less( i, 0, n_points )
        {
            if( input_real( file, &value ) != VIO_OK )
            {
                print_error( "Error reading values.\n" );
                return( 1 );
            }

            fit_this_node[i] = (VIO_SCHAR)
                            (min_value <= value && value <= max_value);
        }

        (void) close_file( file );
    }
    else
        fit_this_node = NULL;

    fit_polygons( n_points, n_neighbours, neighbours, surface_points,
                  model_points, model_weight, centroid_weight, volume, threshold,
                  surface_direction[0], tangent_weight, max_outward, max_inward,
                  fit_this_node,
                  floating_flag, oversample, max_step,
                  n_iters, n_iters_recompute );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    surface = get_polygons_ptr( object_list[0] );
    FREE( surface->points );
    surface->points = surface_points;

    (void) output_graphics_file( output_filename, format, 1, object_list );

    return( 0 );
}



static  int   get_neighbours_neighbours(
    int    node,
    int    n_neighbours[],
    int    *neighbours[],
    int    neigh_indices[] )
{
    int   n, n_nn;
    int   i, nn, neigh, neigh_neigh;

    for_less( n, 0, n_neighbours[node] )
        neigh_indices[n] = neighbours[node][n];
    n_nn = n;

    for_less( n, 0, n_neighbours[node] )
    {
        neigh = neighbours[node][n];
        for_less( nn, 0, n_neighbours[neigh] )
        {
            neigh_neigh = neighbours[neigh][nn];
            if( neigh_neigh == node )
                continue;

            for_less( i, 0, n_nn )
            {
                if( neigh_indices[i] == neigh_neigh )
                    break;
            }

            if( i >= n_nn )
            {
                neigh_indices[n_nn] = neigh_neigh;
                ++n_nn;
            }
        }
    }

    return( n_nn );
}

static  void  create_model_coefficients(
    int              n_nodes,
    int              to_parameter[],
    VIO_Point            surface_points[],
    VIO_Point            model_points[],
    int              n_neighbours[],
    int              *neighbours[],
    int              n_parms_involved[],
    int              *parm_list[],
    ftype            constants[],
    ftype            *node_weights[] )
{
    int              node, eq, dim, dim1, n, ind;
    VIO_Real             *x_flat, *y_flat, *z_flat, con;
    int              *neigh_indices, n_nn, max_neighbours;
    VIO_BOOL          ignoring;
    VIO_Real             *weights[VIO_N_DIMENSIONS][VIO_N_DIMENSIONS];
    VIO_progress_struct  progress;

    max_neighbours = 0;
    for_less( node, 0, n_nodes )
    {
        if( to_parameter[node] >= 0 )
            max_neighbours = MAX( max_neighbours, n_neighbours[node] );
    }

    max_neighbours = MIN( (1+max_neighbours) * max_neighbours, n_nodes );

    ALLOC( neigh_indices, max_neighbours );

    ALLOC( x_flat, max_neighbours );
    ALLOC( y_flat, max_neighbours );
    ALLOC( z_flat, max_neighbours );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    for_less( dim1, 0, VIO_N_DIMENSIONS )
        ALLOC( weights[dim][dim1], max_neighbours );

    initialize_progress_report( &progress, FALSE, n_nodes,
                                "Creating Model Coefficients" );

    eq = 0;
    for_less( node, 0, n_nodes )
    {
        if( to_parameter[node] < 0 )
            continue;

        n_nn = get_neighbours_neighbours( node, n_neighbours, neighbours,
                                          neigh_indices );

        for_less( n, 0, n_nn )
        {
            x_flat[n] = RPoint_x( model_points[neigh_indices[n]] );
            y_flat[n] = RPoint_y( model_points[neigh_indices[n]] );
            z_flat[n] = RPoint_z( model_points[neigh_indices[n]] );
        }

        ignoring = FALSE;
        if( !get_prediction_weights_3d( RPoint_x(model_points[node]),
                                        RPoint_y(model_points[node]),
                                        RPoint_z(model_points[node]),
                                        n_nn, x_flat, y_flat, z_flat,
                                        weights[0], weights[1], weights[2] ) )

        {
            print_error( "Error in interpolation weights, ignoring..\n" );
            ignoring = TRUE;
        }

        if( ignoring )
        {
            n_parms_involved[eq] = 0;
            constants[eq] = (ftype) 0.0;
            ++eq;
            n_parms_involved[eq] = 0;
            constants[eq] = (ftype) 0.0;
            ++eq;
            n_parms_involved[eq] = 0;
            constants[eq] = (ftype) 0.0;
            ++eq;
            continue;
        }

        for_less( dim, 0, VIO_N_DIMENSIONS )
        {
            ind = 1;
            for_less( n, 0, n_nn )
            for_less( dim1, 0, VIO_N_DIMENSIONS )
            {
                if( weights[dim][dim1][n] != 0.0 &&
                    to_parameter[neigh_indices[n]] >= 0 )
                    ++ind;
            }

            n_parms_involved[eq] = ind;

            ALLOC( parm_list[eq], n_parms_involved[eq] );
            ALLOC( node_weights[eq], n_parms_involved[eq] );

            ind = 0;
            parm_list[eq][ind] = VIO_IJ(to_parameter[node],dim,3);
            node_weights[eq][ind] = (ftype) 1.0;
            ++ind;

            con = 0.0;
            for_less( n, 0, n_nn )
            for_less( dim1, 0, VIO_N_DIMENSIONS )
            {
                if( weights[dim][dim1][n] != 0.0 )
                {
                    if( to_parameter[neigh_indices[n]] >= 0 )
                    {
                        parm_list[eq][ind] = VIO_IJ(to_parameter[neigh_indices[n]],
                                                dim1,3);
                        node_weights[eq][ind] = (ftype) -weights[dim][dim1][n];
                        ++ind;
                    }
                    else
                    {
                        con += -weights[dim][dim1][n] *
                            RPoint_coord(surface_points[neigh_indices[n]],dim1);
                    }
                }
            }

            constants[eq] = (ftype) con;

            ++eq;
        }

        update_progress_report( &progress, node+1 );
    }

    terminate_progress_report( &progress );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    for_less( dim1, 0, VIO_N_DIMENSIONS )
        FREE( weights[dim][dim1] );

    FREE( neigh_indices );
    FREE( x_flat );
    FREE( y_flat );
    FREE( z_flat );
}

static  VIO_BOOL  this_is_unique_edge(
    int              node,
    int              neigh,
    int              n_neighbours[],
    int              *neighbours[] )
{
    if( node < neigh )
        return( TRUE );
    else
        return( FALSE );
}

static  void  create_centroid_coefficients(
    int              n_nodes,
    int              to_parameter[],
    VIO_Point            surface_points[],
    VIO_Point            model_points[],
    int              n_neighbours[],
    int              *neighbours[],
    int              n_parms_involved[],
    int              *parm_list[],
    ftype            constants[],
    ftype            *node_weights[] )
{
    int              node, neigh_node, eq, dim, n, n_involved;
    int              neigh_parm_index, ind;
    VIO_Real             con;
    VIO_progress_struct  progress;

    initialize_progress_report( &progress, FALSE, n_nodes,
                                "Creating Stretch Coefficients" );
    eq = 0;
    for_less( node, 0, n_nodes )
    {
        if( to_parameter[node] < 0 )
            continue;

        n_involved = 0;
        for_less( n, 0, n_neighbours[node] )
        {
            neigh_node = neighbours[node][n];
            if( to_parameter[neighbours[node][n]] >= 0 )
                ++n_involved;
        }

        for_less( dim, 0, VIO_N_DIMENSIONS )
        {
            con = 0.0;
            n_parms_involved[eq] = n_involved;
            ALLOC( node_weights[eq], n_parms_involved[eq] );
            ALLOC( parm_list[eq], n_parms_involved[eq] );
            ind = 0;
            node_weights[eq][ind] = (ftype) 1.0;
            parm_list[eq][ind] = VIO_IJ(to_parameter[node],dim,3);
            ++ind;
            for_less( n, 0, n_neighbours[node] )
            {
                neigh_node = neighbours[node][n];
                neigh_parm_index = to_parameter[neighbours[node][n]];
                if( neigh_parm_index >= 0 )
                {
                    node_weights[eq][ind] = (ftype) (-1.0 /
                                      (VIO_Real) n_neighbours[node]);
                    parm_list[eq][ind] = VIO_IJ(neigh_parm_index,dim,3);
                    ++ind;
                }
                else
                    con += -RPoint_coord(model_points[neigh_node],dim)/
                            (VIO_Real) n_neighbours[node];
            }

            constants[eq] = (ftype) con;
            ++eq;
        }

        update_progress_report( &progress, node+1 );
    }

    terminate_progress_report( &progress );
}

static  void  create_image_coefficients(
    VIO_Real                        weight,
    VIO_Volume                      volume,
    voxel_coef_struct           *voxel_lookup,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    boundary_definition_struct  *boundary,
    VIO_Real                        tangent_weight,
    VIO_Real                        max_outward,
    VIO_Real                        max_inward,
    VIO_BOOL                     floating_flag,
    int                         oversample,
    int                         n_nodes,
    int                         n_neighbours[],
    int                         *neighbours[],
    int                         to_parameter[],
    VIO_Real                        parameters[],
    VIO_Point                       surface_points[],
    int                         n_parameters,
    VIO_Real                        *constant,
    ftype                       linear_terms[],
    ftype                       square_terms[],
    int                         n_cross_terms[],
    int                         *cross_parms[],
    ftype                       *cross_terms[],
    int                         alloc_increment )
{
    int        node, n, n_to_do, neigh, n2, neigh2, w, parm_index;
    int        n_involved, inv_index, neigh_parm_index, n_vectors, v;
    int        *indices, dim;
    VIO_Real       dist, dx, dy, dz, value, x, y, z, alpha, cons;
    VIO_Point      origin, p, search_point, p1, p2;
    VIO_Point      neigh_points[1000];
    VIO_Vector     normal, vert, hor, point_normal, search_normal, neigh_normal;
    VIO_Vector     perp, vectors[3];
    VIO_Real       angle, vector_weights[3], *node_weights;
    VIO_Transform  transform;
    VIO_progress_struct  progress;

    ALLOC( indices, n_parameters );
    ALLOC( node_weights, n_parameters );

    initialize_progress_report( &progress, FALSE, n_nodes,
                                "Creating Image Coefficients" );

    for_less( node, 0, n_nodes )
    {
        parm_index = to_parameter[node];
        if( to_parameter[node] < 0 )
            continue;

        for_less( n, 0, n_neighbours[node] )
        {
            neigh = neighbours[node][n];
            neigh_parm_index = to_parameter[neigh];
            if( neigh_parm_index >= 0 )
            {
                fill_Point( neigh_points[n],
                            parameters[VIO_IJ(neigh_parm_index,VIO_X,3)],
                            parameters[VIO_IJ(neigh_parm_index,VIO_Y,3)],
                            parameters[VIO_IJ(neigh_parm_index,VIO_Z,3)] );
            }
            else
                neigh_points[n] = surface_points[neigh];
        }

        find_polygon_normal( n_neighbours[node], neigh_points, &normal );

        fill_Point( origin,
                    parameters[VIO_IJ(parm_index,VIO_X,3)],
                    parameters[VIO_IJ(parm_index,VIO_Y,3)],
                    parameters[VIO_IJ(parm_index,VIO_Z,3)] );

        if( !find_boundary_in_direction( volume, NULL, voxel_lookup,
                                         done_bits, surface_bits,
                                         0.0, &origin, &normal, &normal,
                                         max_outward, max_inward, 0,
                                         boundary, &dist ) )
        {
            if( floating_flag )
            {
                dist = MAX( max_outward, max_inward );
                if( tangent_weight == 0.0 )
                    n_to_do = 1;
                else
                    n_to_do = 3;

                ADD_TO_LSQ( n_parameters, constant, linear_terms, square_terms,
                            n_cross_terms, cross_parms, cross_terms,
                            0, NULL, NULL, weight * (VIO_Real) n_to_do * dist,
                            alloc_increment );

                continue;
            }
            else
                p = origin;
        }
        else
        {
            GET_POINT_ON_RAY( p, origin, normal, dist );
        }

        if( tangent_weight == 1.0 )
        {
            for_less( dim, 0, VIO_N_DIMENSIONS )
            {
                indices[0] = VIO_IJ(parm_index,dim,3);
                node_weights[0] = weight;
                cons = -weight * RPoint_coord( p, dim );
                ADD_TO_LSQ( n_parameters, constant, linear_terms, square_terms,
                            n_cross_terms, cross_parms, cross_terms,
                            1, indices, node_weights, cons, alloc_increment );
            }
        }
        else
        {
            evaluate_volume_in_world( volume,
                                      RPoint_x(p), RPoint_y(p), RPoint_z(p),
                                      0, FALSE, 0.0, &value,
                                      &dx, &dy, &dz,
                                      NULL, NULL, NULL, NULL, NULL, NULL );

            fill_Vector( normal, dx, dy, dz );
            if( null_Vector( &normal ) )
                fill_Vector( normal, 0.0, 0.0, 1.0 );

            NORMALIZE_VECTOR( normal, normal );
        
            vectors[0] = normal;
            vector_weights[0] = weight;
            n_vectors = 1;

            if( tangent_weight > 0.0 )
            {
                create_two_orthogonal_vectors( &normal, &hor, &vert );
                NORMALIZE_VECTOR( hor, hor );
                NORMALIZE_VECTOR( vert, vert );

                vectors[1] = hor;
                vector_weights[1] = tangent_weight * weight;
                vectors[2] = vert;
                vector_weights[2] = tangent_weight * weight;
                n_vectors = 3;
            }

            for_less( v, 0, n_vectors )
            {
                cons = -vector_weights[v] * DOT_POINT_VECTOR( p, vectors[v] );

                node_weights[0] = vector_weights[v] * RVector_x(vectors[v]);
                node_weights[1] = vector_weights[v] * RVector_y(vectors[v]);
                node_weights[2] = vector_weights[v] * RVector_z(vectors[v]);
                indices[0] = VIO_IJ( parm_index, 0, 3 );
                indices[1] = VIO_IJ( parm_index, 1, 3 );
                indices[2] = VIO_IJ( parm_index, 2, 3 );

                ADD_TO_LSQ( n_parameters, constant, linear_terms,
                            square_terms, n_cross_terms, cross_parms,
                            cross_terms, 3, indices, node_weights, cons,
                            alloc_increment );
            }
        }

        update_progress_report( &progress, node+1 );
    }

    terminate_progress_report( &progress );

    if( oversample <= 0 )
    {
        FREE( indices );
        FREE( node_weights );
        return;
    }

    initialize_progress_report( &progress, FALSE, n_nodes,
                                "Creating Oversample Coefficients" );

    for_less( node, 0, n_nodes )
    {
        for_less( n, 0, n_neighbours[node] )
        {
            neigh = neighbours[node][n];
            neigh_parm_index = to_parameter[neigh];
            if( neigh_parm_index >= 0 )
            {
                fill_Point( neigh_points[n],
                            parameters[VIO_IJ(neigh_parm_index,VIO_X,3)],
                            parameters[VIO_IJ(neigh_parm_index,VIO_Y,3)],
                            parameters[VIO_IJ(neigh_parm_index,VIO_Z,3)] );
            }
            else
                neigh_points[n] = surface_points[neigh];
        }

        find_polygon_normal( n_neighbours[node], neigh_points, &point_normal );
        NORMALIZE_VECTOR( point_normal, point_normal );

        for_less( n, 0, n_neighbours[node] )
        {
            neigh = neighbours[node][n];

            if( oversample <= 0 ||
                !this_is_unique_edge( node, neigh, n_neighbours, neighbours ) ||
                (to_parameter[node] < 0 && to_parameter[neigh] < 0 ) )
                continue;

            for_less( n2, 0, n_neighbours[neigh] )
            {
                neigh2 = neighbours[neigh][n2];
                neigh_parm_index = to_parameter[neigh2];
                if( neigh_parm_index >= 0 )
                {
                    fill_Point( neigh_points[n2],
                                parameters[VIO_IJ(neigh_parm_index,VIO_X,3)],
                                parameters[VIO_IJ(neigh_parm_index,VIO_Y,3)],
                                parameters[VIO_IJ(neigh_parm_index,VIO_Z,3)] );
                }
                else
                    neigh_points[n2] = surface_points[neigh2];
            }

            find_polygon_normal( n_neighbours[neigh], neigh_points,
                                 &neigh_normal );
            NORMALIZE_VECTOR( neigh_normal, neigh_normal );

            CROSS_VECTORS( perp, point_normal, neigh_normal );

            if( null_Vector(&perp) )
                angle = 0.0;
            else
            {
                NORMALIZE_VECTOR( perp, perp );
                CROSS_VECTORS( vert, perp, point_normal );
                x = DOT_VECTORS( neigh_normal, point_normal );
                y = DOT_VECTORS( neigh_normal, vert );
                angle = 2.0 * M_PI - compute_clockwise_rotation( x, y );
                if( angle < 0.0 || angle > M_PI )
                    handle_internal_error( "angle < 0.0 || 180.0" );
            }

            n_involved = 0;
            parm_index = to_parameter[node];
            if( parm_index >= 0 )
            {
                fill_Point( p1,
                            parameters[VIO_IJ(parm_index,VIO_X,3)],
                            parameters[VIO_IJ(parm_index,VIO_Y,3)],
                            parameters[VIO_IJ(parm_index,VIO_Z,3)] );
                ++n_involved;
            }
            else
                p1 = surface_points[node];

            neigh_parm_index = to_parameter[neigh];
            if( neigh_parm_index >= 0 )
            {
                fill_Point( p2,
                            parameters[VIO_IJ(neigh_parm_index,VIO_X,3)],
                            parameters[VIO_IJ(neigh_parm_index,VIO_Y,3)],
                            parameters[VIO_IJ(neigh_parm_index,VIO_Z,3)] );
                ++n_involved;
            }
            else
                p2 = surface_points[neigh];

            for_less( w, 0, oversample )
            {
                alpha = (VIO_Real) (w+1) / (VIO_Real) (oversample+1);

                if( angle == 0.0 )
                    search_normal = point_normal;
                else
                {
                    make_rotation_about_axis( &perp,
                                              (VIO_Real) alpha * angle,
                                              &transform );

                    transform_vector( &transform,
                                      (VIO_Real) Vector_x(point_normal),
                                      (VIO_Real) Vector_y(point_normal),
                                      (VIO_Real) Vector_z(point_normal),
                                      &x, &y, &z );
                    fill_Vector( search_normal, x, y, z );
                }

                fill_Point( search_point, (1.0 - alpha) * RPoint_x(p1) +
                                                 alpha  * RPoint_x(p2),
                                          (1.0 - alpha) * RPoint_y(p1) +
                                                 alpha  * RPoint_y(p2),
                                          (1.0 - alpha) * RPoint_z(p1) +
                                                 alpha  * RPoint_z(p2) );

                if( !find_boundary_in_direction( volume, NULL, voxel_lookup,
                                                 done_bits, surface_bits,
                                                 0.0, &search_point,
                                                 &search_normal, &search_normal,
                                                 max_outward, max_inward, 0,
                                                 boundary, &dist ) )
                {
                    if( floating_flag )
                    {
                        dist = MAX( max_outward, max_inward );
                        if( tangent_weight == 0.0 )
                            n_to_do = 1;
                        else
                            n_to_do = 3;

                        ADD_TO_LSQ( n_parameters, constant, linear_terms,
                                    square_terms, n_cross_terms,
                                    cross_parms, cross_terms,
                                    0, NULL, NULL,
                                    weight * (VIO_Real) n_to_do * dist,
                                    alloc_increment );

                        continue;
                    }
                    else
                        p = search_point;
                }
                else
                {
                    GET_POINT_ON_RAY( p, search_point, search_normal, dist );
                }

                if( tangent_weight == 1.0 )
                {
                    for_less( dim, 0, VIO_N_DIMENSIONS )
                    {
                        inv_index = 0;
                        cons = -weight * RPoint_coord( p, dim );

                        if( parm_index >= 0 )
                        {
                            node_weights[inv_index] = weight * (1.0 - alpha);
                            indices[inv_index] = VIO_IJ(parm_index,dim,3);
                            ++inv_index;
                        }
                        else
                            cons += weight * (1.0 - alpha) *
                                    RPoint_coord(p1,dim);

                        if( neigh_parm_index >= 0 )
                        {
                            node_weights[inv_index] = weight * alpha;
                            indices[inv_index] = VIO_IJ(neigh_parm_index,dim,3);
                            ++inv_index;
                        }
                        else
                            cons += weight * alpha * RPoint_coord(p2,dim);

                        ADD_TO_LSQ( n_parameters, constant, linear_terms,
                                    square_terms,
                                    n_cross_terms, cross_parms, cross_terms,
                                    inv_index, indices, node_weights, cons,
                                    alloc_increment );
                    }
                }
                else
                {
                    evaluate_volume_in_world( volume,
                                          RPoint_x(p), RPoint_y(p), RPoint_z(p),
                                          0, FALSE, 0.0, &value,
                                          &dx, &dy, &dz,
                                          NULL, NULL, NULL, NULL, NULL, NULL );

                    fill_Vector( normal, dx, dy, dz );
                    if( null_Vector( &normal ) )
                        fill_Vector( normal, 0.0, 0.0, 1.0 );

                    NORMALIZE_VECTOR( normal, normal );

                    vectors[0] = normal;
                    vector_weights[0] = weight;
                    n_vectors = 1;

                    if( tangent_weight > 0.0 )
                    {
                        create_two_orthogonal_vectors( &normal, &hor, &vert );
                        NORMALIZE_VECTOR( hor, hor );
                        NORMALIZE_VECTOR( vert, vert );

                        vectors[1] = hor;
                        vector_weights[1] = weight * tangent_weight;
                        vectors[2] = vert;
                        vector_weights[2] = weight * tangent_weight;
                        n_vectors = 3;
                    }

                    for_less( v, 0, n_vectors )
                    {
                        inv_index = 0;
                        cons = -vector_weights[v] *
                                DOT_POINT_VECTOR( p, vectors[v] );

                        if( parm_index >= 0 )
                        {
                            node_weights[0] = vector_weights[v] * (1.0 - alpha)
                                              * RVector_x(vectors[v]);
                            node_weights[1] = vector_weights[v] * (1.0 - alpha)
                                              * RVector_y(vectors[v]);
                            node_weights[2] = vector_weights[v] * (1.0 - alpha)
                                              * RVector_z(vectors[v]);
                            indices[0] = VIO_IJ( parm_index, 0, 3 );
                            indices[1] = VIO_IJ( parm_index, 1, 3 );
                            indices[2] = VIO_IJ( parm_index, 2, 3 );
                            inv_index += 3;
                        }
                        else
                            cons += vector_weights[v] * (1.0 - alpha) *
                                          DOT_POINT_VECTOR( p1, vectors[v] );

                        if( neigh_parm_index >= 0 )
                        {
                            node_weights[inv_index+0] = vector_weights[v] *
                                               alpha * RVector_x(vectors[v]);
                            node_weights[inv_index+1] = vector_weights[v] *
                                               alpha * RVector_y(vectors[v]);
                            node_weights[inv_index+2] = vector_weights[v] *
                                               alpha * RVector_z(vectors[v]);
                            indices[inv_index+0] = VIO_IJ( neigh_parm_index, 0, 3 );
                            indices[inv_index+1] = VIO_IJ( neigh_parm_index, 1, 3 );
                            indices[inv_index+2] = VIO_IJ( neigh_parm_index, 2, 3 );
                            inv_index += 3;
                        }
                        else
                            cons += vector_weights[v] * alpha *
                                          DOT_POINT_VECTOR( p2, vectors[v] );

                        ADD_TO_LSQ( n_parameters, constant, linear_terms,
                                    square_terms, n_cross_terms, cross_parms,
                                    cross_terms, inv_index, indices,
                                    node_weights, cons,
                                    alloc_increment );
                    }
                }
            }
        }

        update_progress_report( &progress, node+1 );
    }

    terminate_progress_report( &progress );

    FREE( indices );
    FREE( node_weights );
}

static  void   fit_polygons(
    int                n_points,
    int                n_neighbours[],
    int                *neighbours[],
    VIO_Point              surface_points[],
    VIO_Point              model_points[],
    VIO_Real               model_weight,
    VIO_Real               centroid_weight,
    VIO_Volume             volume,
    VIO_Real               threshold,
    char               normal_direction,
    VIO_Real               tangent_weight,
    VIO_Real               max_outward,
    VIO_Real               max_inward,
    VIO_SCHAR       fit_this_node[],
    VIO_BOOL            floating_flag,
    int                oversample,
    VIO_Real               max_step,
    int                n_iters,
    int                n_iters_recompute )
{
    int                         eq, point, n, iter, n_parameters;
    int                         n_model_equations, n_image_equations;
    int                         n_equations, *n_parms_involved, **parm_list;
    int                         n_image_per_point;
    int                         n_oversample_equations;
    int                         n_moving_points, alloc_increment;
    int                         sizes[VIO_N_DIMENSIONS];
    int                         *to_parameter;
    int                         parm_index, p;
    int                         n_centroid_equations;
    ftype                       *constants, **node_weights;
    VIO_Real                        *parameters, *weights, weight;
    polygons_struct             save_p;
    boundary_definition_struct  boundary;
    voxel_coef_struct           voxel_lookup;
    bitlist_3d_struct           done_bits, surface_bits;
    bitlist_3d_struct           *done_bits_ptr, *surface_bits_ptr;
    VIO_Real                        constant;
    int                         *n_cross_terms, **cross_parms;
    ftype                       *linear_terms, *square_terms, **cross_terms;

    set_boundary_definition( &boundary, threshold, threshold, -1.0, 90.0,
                             normal_direction, 1.0e-4 );

    if( tangent_weight > 0.0 )
        n_image_per_point = 3;
    else
        n_image_per_point = 1;

    ALLOC( to_parameter, n_points );

    n_moving_points = 0;
    for_less( point, 0, n_points )
    {
        if( fit_this_node == NULL || fit_this_node[point] )
        {
            to_parameter[point] = n_moving_points;
            ++n_moving_points;
        }
        else
            to_parameter[point] = -1;
    }

    n_parameters = 3 * n_moving_points;

    n_model_equations = 3 * n_moving_points;
    n_image_equations = n_image_per_point * n_moving_points;

    n_oversample_equations = 0;
    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            if( this_is_unique_edge( point, neighbours[point][n],
                                     n_neighbours, neighbours ) &&
                (to_parameter[point] >= 0 ||
                 to_parameter[neighbours[point][n]] >= 0) )
            {
                n_oversample_equations += oversample;
            }
        }
    }
    n_oversample_equations *= n_image_per_point;

    if( centroid_weight > 0.0 )
        n_centroid_equations = 3 * n_moving_points;
    else
        n_centroid_equations = 0;

    n_equations = n_model_equations + n_centroid_equations;

    ALLOC( n_parms_involved, n_equations );
    ALLOC( constants, n_equations );
    ALLOC( node_weights, n_equations );
    ALLOC( parm_list, n_equations );

    create_model_coefficients( n_points, to_parameter, surface_points,
                               model_points, n_neighbours, neighbours,
                               n_parms_involved,
                               parm_list, constants, node_weights );

    model_weight = sqrt( model_weight / (VIO_Real) n_model_equations );

    for_less( eq, 0, n_model_equations )
    {
        constants[eq] *= (ftype) model_weight;
        for_less( n, 0, n_parms_involved[eq] )
            node_weights[eq][n] *= (ftype) model_weight;
    }

    if( centroid_weight > 0.0 )
    {
        create_centroid_coefficients( n_points, to_parameter, surface_points,
                                     model_points, n_neighbours, neighbours,
                                     &n_parms_involved[n_model_equations],
                                     &parm_list[n_model_equations],
                                     &constants[n_model_equations],
                                     &node_weights[n_model_equations] );

        centroid_weight = sqrt( centroid_weight / (VIO_Real) n_centroid_equations );

        for_less( eq, n_model_equations,
                      n_model_equations + n_centroid_equations )
        {
            constants[eq] *= (ftype) centroid_weight;
            for_less( n, 0, n_parms_involved[eq] )
                node_weights[eq][n] *= (ftype) centroid_weight;
        }
    }

    ALLOC( parameters, n_parameters );

    for_less( point, 0, n_points )
    {
        parm_index = to_parameter[point];
        if( parm_index >= 0 )
        {
            parameters[VIO_IJ(parm_index,VIO_X,3)] = RPoint_x(surface_points[point]);
            parameters[VIO_IJ(parm_index,VIO_Y,3)] = RPoint_y(surface_points[point]);
            parameters[VIO_IJ(parm_index,VIO_Z,3)] = RPoint_z(surface_points[point]);
        }
    }

    initialize_lookup_volume_coeficients( &voxel_lookup );
    get_volume_sizes( volume, sizes );

    done_bits_ptr = &done_bits;
    surface_bits_ptr = &surface_bits;

    if( done_bits_ptr != NULL )
    {
        create_bitlist_3d( sizes[VIO_X], sizes[VIO_Y], sizes[VIO_Z], &done_bits );
        create_bitlist_3d( sizes[VIO_X], sizes[VIO_Y], sizes[VIO_Z], &surface_bits );
    }

    ALLOC( weights, n_parameters );

    INITIALIZE_LSQ( n_parameters, &constant, &linear_terms,
                    &square_terms, &n_cross_terms, &cross_parms, &cross_terms );
    alloc_increment = 5;

    iter = 0;
    while( iter < n_iters )
    {
        RESET_LSQ( n_parameters, &constant, linear_terms,
                   square_terms, n_cross_terms, cross_parms, cross_terms );

        for_less( eq, 0, n_model_equations + n_centroid_equations )
        {
            for_less( p, 0, n_parms_involved[eq] )
                weights[p] = (VIO_Real) node_weights[eq][p];

            ADD_TO_LSQ( n_parameters, &constant, linear_terms,
                        square_terms, n_cross_terms, cross_parms, cross_terms,
                        n_parms_involved[eq], parm_list[eq], weights,
                        (VIO_Real) constants[eq], alloc_increment );
        }

        weight = sqrt( 1.0 / (VIO_Real) (n_image_equations+n_oversample_equations));

        create_image_coefficients( weight, volume, &voxel_lookup,
                       done_bits_ptr, surface_bits_ptr,
                       &boundary, tangent_weight,
                       max_outward, max_inward,
                       floating_flag, oversample,
                       n_points, n_neighbours, neighbours,
                       to_parameter, parameters, surface_points,
                       n_parameters, &constant, linear_terms,
                       square_terms, n_cross_terms, cross_parms, cross_terms,
                       alloc_increment );

        if( iter == 0 )
        {
            REALLOC_LSQ( n_parameters, n_cross_terms, cross_parms,
                         cross_terms );
            alloc_increment = 1;
        }

        (void) MINIMIZE_LSQ( n_parameters, constant, linear_terms,
                             square_terms, n_cross_terms, cross_parms,
                             cross_terms, max_step, n_iters_recompute,
                             parameters );

        iter += n_iters_recompute;
        print( "########### %d:\n", iter );
        (void) flush_file( stdout );
    }

    FREE( weights );

    DELETE_LSQ( n_parameters, linear_terms,
                square_terms, n_cross_terms, cross_parms, cross_terms );

    for_less( point, 0, n_points )
    {
        parm_index = to_parameter[point];
        if( parm_index >= 0 )
        {
            fill_Point( surface_points[point],
                        parameters[VIO_IJ(parm_index,VIO_X,3)],
                        parameters[VIO_IJ(parm_index,VIO_Y,3)],
                        parameters[VIO_IJ(parm_index,VIO_Z,3)] )
        }
    }

    save_p.n_points = n_points;
    delete_polygon_point_neighbours( &save_p, n_neighbours,
                                     neighbours, NULL, NULL );

    FREE( to_parameter );
    FREE( parameters );

    for_less( eq, 0, n_equations )
    {
        FREE( parm_list[eq] );
        FREE( node_weights[eq] );
    }

    FREE( n_parms_involved );
    FREE( constants );
    FREE( node_weights );
    FREE( parm_list );
    FREE( model_points );
}
