#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <deform.h>

#undef   USING_FLOAT
#define  USING_FLOAT

#ifdef USING_FLOAT
typedef  float  ftype;
#else
typedef  Real   ftype;
#endif

private  void   fit_polygons(
    int                n_points,
    int                n_neighbours[],
    int                *neighbours[],
    Point              surface_points[],
    Point              model_points[],
    Real               model_weight,
    Volume             volume,
    Real               threshold,
    char               normal_direction,
    Real               tangent_weight,
    Real               max_outward,
    Real               max_inward,
    BOOLEAN            floating_flag,
    int                oversample,
    int                n_iters,
    int                n_iters_recompute );

private  void  usage(
    char   executable_name[] )
{
    STRING  usage_format = "\
Usage:     %s  input.obj output.obj model.obj model_weight volume.mnc \n\
                  threshold +|-|0  tangent_weight out_dist in_dist\n\
                  float/nofloat oversample\n\
                  [n_iters] [n_between]\n\n";

    print_error( usage_format, executable_name );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               input_filename, output_filename, model_filename;
    STRING               surface_direction, volume_filename, floating_string;
    int                  n_objects, n_m_objects;
    int                  n_iters, n_iters_recompute, n_points;
    int                  *n_neighbours, **neighbours, oversample;
    File_formats         format;
    object_struct        **object_list, **m_object_list;
    polygons_struct      *surface, *model_surface;
    Volume               volume;
    Point                *surface_points, *model_points;
    Real                 threshold, model_weight;
    Real                 max_outward, max_inward, tangent_weight;
    BOOLEAN              floating_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &model_filename ) ||
        !get_real_argument( 0.0, &model_weight ) ||
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

    floating_flag = equal_strings( floating_string, "float" );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
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
                             &m_object_list ) != OK || n_m_objects != 1 ||
        get_object_type(m_object_list[0]) != POLYGONS )
        return( 1 );

    model_surface = get_polygons_ptr( m_object_list[0] );
    model_points = model_surface->points;
    ALLOC( model_surface->points, 1 );
    delete_object_list( n_m_objects, m_object_list );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    fit_polygons( n_points, n_neighbours, neighbours, surface_points,
                  model_points, model_weight, volume, threshold,
                  surface_direction[0], tangent_weight, max_outward, max_inward,
                  floating_flag, oversample, n_iters, n_iters_recompute );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    surface = get_polygons_ptr( object_list[0] );
    FREE( surface->points );
    surface->points = surface_points;

    (void) output_graphics_file( output_filename, format, 1, object_list );

    return( 0 );
}

private  int   get_neighbours_neighbours(
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

private  void  create_model_coefficients(
    int              n_nodes,
    Point            model_points[],
    int              n_neighbours[],
    int              *neighbours[],
    int              n_parms_involved[],
    int              *parm_list[],
    ftype            constants[],
    ftype            *node_weights[] )
{
    int              node, eq, dim, dim1, n, ind;
    FILE             *file;
    Real             *x_flat;
    Real             *y_flat;
    Real             *z_flat;
    int              *neigh_indices, n_nn, max_neighbours;
    BOOLEAN          ignoring;
    Real             *weights[N_DIMENSIONS][N_DIMENSIONS];
    progress_struct  progress;

    if( getenv( "LOAD_MODEL_COEFS" ) != NULL &&
        open_file( getenv( "LOAD_MODEL_COEFS" ), READ_FILE, BINARY_FORMAT,
                   &file ) == OK )
    {
        for_less( eq, 0, 3 * n_nodes )
        {
            int  p;

#ifdef USING_FLOAT
            (void) io_float( file, READ_FILE, BINARY_FORMAT, &constants[eq] );
#else
            (void) io_real( file, READ_FILE, BINARY_FORMAT, &constants[eq] );
#endif
            (void) io_int( file, READ_FILE, BINARY_FORMAT,
                           &n_parms_involved[eq] );
            ALLOC( parm_list[eq], n_parms_involved[eq] );
            ALLOC( node_weights[eq], n_parms_involved[eq] );
            for_less( p, 0, n_parms_involved[eq] )
            {
                (void) io_int( file, READ_FILE, BINARY_FORMAT,
                                &parm_list[eq][p] );
#ifdef USING_FLOAT
                (void) io_float( file, READ_FILE, BINARY_FORMAT,
                                &node_weights[eq][p] );
#else
                (void) io_real( file, READ_FILE, BINARY_FORMAT,
                                &node_weights[eq][p] );
#endif
            }
        }
        close_file( file );
        return;
    }


    max_neighbours = 0;
    for_less( node, 0, n_nodes )
        max_neighbours = MAX( max_neighbours, n_neighbours[node] );

    max_neighbours = MIN( (1+max_neighbours) * max_neighbours, n_nodes );

    ALLOC( neigh_indices, max_neighbours );

    ALLOC( x_flat, max_neighbours );
    ALLOC( y_flat, max_neighbours );
    ALLOC( z_flat, max_neighbours );

    for_less( dim, 0, N_DIMENSIONS )
    for_less( dim1, 0, N_DIMENSIONS )
        ALLOC( weights[dim][dim1], max_neighbours );

    initialize_progress_report( &progress, FALSE, n_nodes,
                                "Creating Model Coefficients" );

    eq = 0;
    for_less( node, 0, n_nodes )
    {
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

        for_less( dim, 0, N_DIMENSIONS )
        {
            ind = 1;
            for_less( n, 0, n_nn )
            for_less( dim1, 0, N_DIMENSIONS )
            {
                if( weights[dim][dim1][n] != 0.0 )
                    ++ind;
            }

            constants[eq] = (ftype) 0.0;
            n_parms_involved[eq] = ind;

            ALLOC( parm_list[eq], n_parms_involved[eq] );
            ALLOC( node_weights[eq], n_parms_involved[eq] );

            ind = 0;
            parm_list[eq][ind] = IJ(node,dim,3);
            node_weights[eq][ind] = (ftype) 1.0;
            ++ind;

            for_less( n, 0, n_nn )
            for_less( dim1, 0, N_DIMENSIONS )
            {
                if( weights[dim][dim1][n] != 0.0 )
                {
                    parm_list[eq][ind] = IJ(neigh_indices[n],dim1,3);
                    node_weights[eq][ind] = (ftype) -weights[dim][dim1][n];
                    ++ind;
                }
            }

            ++eq;
        }

        update_progress_report( &progress, node+1 );
    }

    terminate_progress_report( &progress );

    if( getenv( "SAVE_MODEL_COEFS" ) != NULL &&
        open_file( getenv( "SAVE_MODEL_COEFS" ), WRITE_FILE, BINARY_FORMAT,
                   &file ) == OK )
    {
        for_less( eq, 0, 3 * n_nodes )
        {
            int  p;

#ifdef USING_FLOAT
            (void) io_float( file, WRITE_FILE, BINARY_FORMAT, &constants[eq] );
#else
            (void) io_real( file, WRITE_FILE, BINARY_FORMAT, &constants[eq] );
#endif
            (void) io_int( file, WRITE_FILE, BINARY_FORMAT,
                           &n_parms_involved[eq] );
            for_less( p, 0, n_parms_involved[eq] )
            {
                (void) io_int( file, WRITE_FILE, BINARY_FORMAT,
                                &parm_list[eq][p] );
#ifdef USING_FLOAT
                (void) io_float( file, WRITE_FILE, BINARY_FORMAT,
                                 &node_weights[eq][p] );
#else
                (void) io_real( file, WRITE_FILE, BINARY_FORMAT,
                                &node_weights[eq][p] );
#endif
            }
        }
        close_file( file );
    }

#ifdef DEBUG
#define DEBUG
#undef DEBUG
    for_less( eq, 0, 3 * n_nodes )
    {
        int  p;
        print( "%3d: %g : ", eq, constants[eq] );
        for_less( p, 0, n_parms_involved[eq] )
        {
            print( " %d:%g ", parm_list[eq][p], node_weights[eq][p] );
        }
        print( "\n" );
    }
#endif

    for_less( dim, 0, N_DIMENSIONS )
    for_less( dim1, 0, N_DIMENSIONS )
        FREE( weights[dim][dim1] );

    FREE( neigh_indices );
    FREE( x_flat );
    FREE( y_flat );
    FREE( z_flat );
}

private  BOOLEAN  this_is_unique_edge(
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

private  void  create_image_coefficients(
    Volume                      volume,
    boundary_definition_struct  *boundary,
    Real                        tangent_weight,
    Real                        max_outward,
    Real                        max_inward,
    BOOLEAN                     floating_flag,
    int                         oversample,
    int                         n_nodes,
    int                         n_neighbours[],
    int                         *neighbours[],
    Real                        parameters[],
    int                         *parm_list[],
    ftype                       constants[],
    ftype                       *node_weights[] )
{
    int        eq, node, n, n_to_do, neigh, n2, neigh2, w;
    Real       dist, dx, dy, dz, value, x, y, z, alpha;
    Point      origin, p, search_point, p1, p2;
    Point      neigh_points[1000];
    Vector     normal, vert, hor, point_normal, search_normal, neigh_normal;
    Vector     perp;
    Real       angle;
    Transform  transform;

    eq = 0;

    for_less( node, 0, n_nodes )
    {
        for_less( n, 0, n_neighbours[node] )
        {
            fill_Point( neigh_points[n],
                        parameters[IJ(neighbours[node][n],X,3)],
                        parameters[IJ(neighbours[node][n],Y,3)],
                        parameters[IJ(neighbours[node][n],Z,3)] );
        }

        find_polygon_normal( n_neighbours[node], neigh_points, &normal );

        fill_Point( origin,
                    parameters[IJ(node,X,3)],
                    parameters[IJ(node,Y,3)],
                    parameters[IJ(node,Z,3)] );

        if( !find_boundary_in_direction( volume, NULL, NULL, NULL, NULL,
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

                for_less( n, 0, n_to_do )
                {
                    if( tangent_weight == 1.0 )
                    {
                        node_weights[eq][0] = (ftype) 0.0;
                        node_weights[eq][1] = (ftype) 0.0;
                        node_weights[eq][2] = (ftype) 0.0;
                    }
                    else
                    {
                        node_weights[eq][0] = (ftype) 0.0;
                    }

                    constants[eq] = (ftype) (dist * dist);
                    ++eq;
                }

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
            node_weights[eq][0] = (ftype) 1.0;
            constants[eq] = (ftype) -RPoint_x(p);
            ++eq;
            node_weights[eq][0] = (ftype) 1.0;
            constants[eq] = (ftype) -RPoint_y(p);
            ++eq;
            node_weights[eq][0] = (ftype) 1.0;
            constants[eq] = (ftype) -RPoint_z(p);
            ++eq;
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
        
            node_weights[eq][0] = (ftype) RVector_x(normal);
            node_weights[eq][1] = (ftype) RVector_y(normal);
            node_weights[eq][2] = (ftype) RVector_z(normal);
            constants[eq] = (ftype) -DOT_POINT_VECTOR( p, normal );
            ++eq;

            if( tangent_weight > 0.0 )
            {
                create_two_orthogonal_vectors( &normal, &hor, &vert );
                NORMALIZE_VECTOR( hor, hor );
                NORMALIZE_VECTOR( vert, vert );

                node_weights[eq][0] = (ftype) (tangent_weight * RVector_x(hor));
                node_weights[eq][1] = (ftype) (tangent_weight * RVector_y(hor));
                node_weights[eq][2] = (ftype) (tangent_weight * RVector_z(hor));
                constants[eq] = (ftype) (-tangent_weight *
                                         DOT_POINT_VECTOR( p, hor ));
                ++eq;

                node_weights[eq][0] = (ftype) (tangent_weight *RVector_x(vert));
                node_weights[eq][1] = (ftype) (tangent_weight *RVector_y(vert));
                node_weights[eq][2] = (ftype) (tangent_weight *RVector_z(vert));
                constants[eq] = (ftype) (-tangent_weight *
                                         DOT_POINT_VECTOR( p, vert ));
                ++eq;
            }
        }
    }

    if( oversample <= 0 )
        return;

    for_less( node, 0, n_nodes )
    {
        for_less( n, 0, n_neighbours[node] )
        {
            neigh = neighbours[node][n];
            fill_Point( neigh_points[n],
                        parameters[IJ(neigh,X,3)],
                        parameters[IJ(neigh,Y,3)],
                        parameters[IJ(neigh,Z,3)] );
        }

        find_polygon_normal( n_neighbours[node], neigh_points, &point_normal );
        NORMALIZE_VECTOR( point_normal, point_normal );

        for_less( n, 0, n_neighbours[node] )
        {
            neigh = neighbours[node][n];

            if( oversample <= 0 ||
                !this_is_unique_edge( node, neigh, n_neighbours, neighbours ) )
            continue;

            for_less( n2, 0, n_neighbours[neigh] )
            {
                neigh2 = neighbours[neigh][n2];
                fill_Point( neigh_points[n2],
                            parameters[IJ(neigh2,X,3)],
                            parameters[IJ(neigh2,Y,3)],
                            parameters[IJ(neigh2,Z,3)] );
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
                angle = 2.0 * PI - compute_clockwise_rotation( x, y );
                if( angle < 0.0 || angle > PI )
                    handle_internal_error( "angle < 0.0 || 180.0" );
            }

            fill_Point( p1,
                        parameters[IJ(node,X,3)],
                        parameters[IJ(node,Y,3)],
                        parameters[IJ(node,Z,3)] );
            fill_Point( p2,
                        parameters[IJ(neigh,X,3)],
                        parameters[IJ(neigh,Y,3)],
                        parameters[IJ(neigh,Z,3)] );

            for_less( w, 0, oversample )
            {
                alpha = (Real) (w+1) / (Real) (oversample+1);

                if( angle == 0.0 )
                    search_normal = point_normal;
                else
                {
                    make_rotation_about_axis( &perp,
                                              (Real) alpha * angle,
                                              &transform );

                    transform_vector( &transform,
                                      (Real) Vector_x(point_normal),

                                     (Real) Vector_y(point_normal),
                                      (Real) Vector_z(point_normal),
                                      &x, &y, &z );
                    fill_Vector( search_normal, x, y, z );
                }

                fill_Point( search_point, (1.0 - alpha) * RPoint_x(p1) +
                                                 alpha  * RPoint_x(p2),
                                          (1.0 - alpha) * RPoint_y(p1) +
                                                 alpha  * RPoint_y(p2),
                                          (1.0 - alpha) * RPoint_z(p1) +
                                                 alpha  * RPoint_z(p2) );

                if( !find_boundary_in_direction( volume, NULL, NULL, NULL, NULL,
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

                        for_less( n, 0, n_to_do )
                        {
                            if( tangent_weight == 1.0 )
                            {
                                node_weights[eq][0] = (ftype) 0.0;
                                node_weights[eq][1] = (ftype) 0.0;
                            }
                            else
                            {
                                node_weights[eq][0] = (ftype) 0.0;
                                node_weights[eq][1] = (ftype) 0.0;
                                node_weights[eq][2] = (ftype) 0.0;
                                node_weights[eq][3] = (ftype) 0.0;
                                node_weights[eq][4] = (ftype) 0.0;
                                node_weights[eq][5] = (ftype) 0.0;
                            }

                            constants[eq] = (ftype) (dist * dist);
                            ++eq;
                        }

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
                    node_weights[eq][0] = (ftype) (1.0 - alpha);
                    node_weights[eq][1] = (ftype) alpha;
                    constants[eq] = (ftype) -RPoint_x(p);
                    ++eq;
                    node_weights[eq][0] = (ftype) (1.0 - alpha);
                    node_weights[eq][1] = (ftype) alpha;
                    constants[eq] = (ftype) -RPoint_y(p);
                    ++eq;
                    node_weights[eq][0] = (ftype) (1.0 - alpha);
                    node_weights[eq][1] = (ftype) alpha;
                    constants[eq] = (ftype) -RPoint_z(p);
                    ++eq;
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
        
                    node_weights[eq][0] =(ftype)((1.0 - alpha) * RVector_x(normal));
                    node_weights[eq][1] =(ftype)((1.0 - alpha) * RVector_y(normal));
                    node_weights[eq][2] =(ftype)((1.0 - alpha) * RVector_z(normal));
                    node_weights[eq][3] =(ftype)(       alpha  * RVector_x(normal));
                    node_weights[eq][4] =(ftype)(       alpha  * RVector_y(normal));
                    node_weights[eq][5] =(ftype)(       alpha  * RVector_z(normal));
                    constants[eq] = (ftype) -DOT_POINT_VECTOR( p, normal );
                    ++eq;

                    if( tangent_weight > 0.0 )
                    {
                        create_two_orthogonal_vectors( &normal, &hor, &vert );
                        NORMALIZE_VECTOR( hor, hor );
                        NORMALIZE_VECTOR( vert, vert );

                        node_weights[eq][0] =
                         (ftype) ((1.0 - alpha) * tangent_weight * RVector_x(hor));
                        node_weights[eq][1] =
                         (ftype) ((1.0 - alpha) * tangent_weight * RVector_y(hor));
                        node_weights[eq][2] =
                         (ftype) ((1.0 - alpha) * tangent_weight * RVector_z(hor));
                        node_weights[eq][3] =
                         (ftype) (alpha * tangent_weight * RVector_x(hor));
                        node_weights[eq][4] =
                         (ftype) (alpha * tangent_weight * RVector_y(hor));
                        node_weights[eq][5] =
                         (ftype) (alpha * tangent_weight * RVector_z(hor));
                        constants[eq] = (ftype) (-tangent_weight *
                                                 DOT_POINT_VECTOR( p, hor ));
                        ++eq;

                        node_weights[eq][0] =
                         (ftype) ((1.0 - alpha) * tangent_weight * RVector_x(vert));
                        node_weights[eq][1] =
                         (ftype) ((1.0 - alpha) * tangent_weight * RVector_y(vert));
                        node_weights[eq][2] =
                         (ftype) ((1.0 - alpha) * tangent_weight * RVector_z(vert));
                        node_weights[eq][3] =
                         (ftype) ( alpha * tangent_weight * RVector_x(vert));
                        node_weights[eq][4] =
                         (ftype) ( alpha * tangent_weight * RVector_y(vert));
                        node_weights[eq][5] =
                         (ftype) ( alpha * tangent_weight * RVector_z(vert));
                        constants[eq] = (ftype) (-tangent_weight *
                                                 DOT_POINT_VECTOR( p, vert ));
                        ++eq;
                    }
                }
            }
        }
    }
}

private  void   fit_polygons(
    int                n_points,
    int                n_neighbours[],
    int                *neighbours[],
    Point              surface_points[],
    Point              model_points[],
    Real               model_weight,
    Volume             volume,
    Real               threshold,
    char               normal_direction,
    Real               tangent_weight,
    Real               max_outward,
    Real               max_inward,
    BOOLEAN            floating_flag,
    int                oversample,
    int                n_iters,
    int                n_iters_recompute )
{
    int                         eq, point, n, iter, which, o;
    int                         n_model_equations, n_image_equations;
    int                         n_equations, *n_parms_involved, **parm_list;
    int                         n_image_per_point;
    int                         n_oversample_equations, n_bound_nodes;
    ftype                       *constants, **node_weights;
    Real                        *parameters;
    polygons_struct             save_p;
    boundary_definition_struct  boundary;

    set_boundary_definition( &boundary, threshold, threshold, -1.0, 90.0,
                             normal_direction, 1.0e-4 );

    n_model_equations = 3 * n_points;

    if( tangent_weight > 0.0 )
        n_image_per_point = 3;
    else
        n_image_per_point = 1;

    if( tangent_weight == 1.0 )
        n_bound_nodes = 1;
    else
        n_bound_nodes = 3;

    n_image_equations = n_image_per_point * n_points;

    n_oversample_equations = 0;
    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            if( this_is_unique_edge( point, neighbours[point][n],
                                     n_neighbours, neighbours ) )
                n_oversample_equations += oversample;
        }
    }
    n_oversample_equations *= n_image_per_point;

    n_equations = n_model_equations + n_image_equations +
                  n_oversample_equations;

    ALLOC( n_parms_involved, n_equations );
    ALLOC( constants, n_equations );
    ALLOC( node_weights, n_equations );
    ALLOC( parm_list, n_equations );

    create_model_coefficients( n_points, model_points, n_neighbours, neighbours,
                               n_parms_involved,
                               parm_list, constants, node_weights );

    FREE( model_points );

    model_weight = sqrt( model_weight );
    for_less( eq, 0, n_model_equations )
    {
        constants[eq] *= (ftype) model_weight;
        for_less( n, 0, n_parms_involved[eq] )
            node_weights[eq][n] *= (ftype) model_weight;
    }

    for_less( point, 0, n_points )
    {
        for_less( which, 0, n_image_per_point )
        {
            eq = n_model_equations + IJ(point,which,n_image_per_point);
            n_parms_involved[eq] = n_bound_nodes;
            ALLOC( node_weights[eq], n_parms_involved[eq] );
            ALLOC( parm_list[eq], n_parms_involved[eq] );
            if( n_bound_nodes == 1 )
            {
                parm_list[eq][0] = IJ(point,which,3);
            }
            else
            {
                parm_list[eq][0] = IJ(point,X,3);
                parm_list[eq][1] = IJ(point,Y,3);
                parm_list[eq][2] = IJ(point,Z,3);
            }
        }
    }

    eq = n_model_equations + n_image_equations;
    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            if( oversample > 0 &&
                this_is_unique_edge( point, neighbours[point][n],
                                     n_neighbours, neighbours ) )
            {
                for_less( o, 0, oversample )
                for_less( which, 0, n_image_per_point )
                {
                    n_parms_involved[eq] = 2 * n_bound_nodes;
                    ALLOC( node_weights[eq], n_parms_involved[eq] );
                    ALLOC( parm_list[eq], n_parms_involved[eq] );
                    if( n_bound_nodes == 1 )
                    {
                        parm_list[eq][0] = IJ(point,which,3);
                        parm_list[eq][1] = IJ(neighbours[point][n],which,3);
                    }
                    else
                    {
                        parm_list[eq][0] = IJ(point,X,3);
                        parm_list[eq][1] = IJ(point,Y,3);
                        parm_list[eq][2] = IJ(point,Z,3);
                        parm_list[eq][3] = IJ(neighbours[point][n],X,3);
                        parm_list[eq][4] = IJ(neighbours[point][n],Y,3);
                        parm_list[eq][5] = IJ(neighbours[point][n],Z,3);
                    }

                    ++eq;
                }
            }
        }
    }

    ALLOC( parameters, 3 * n_points );

    for_less( point, 0, n_points )
    {
        parameters[IJ(point,X,3)] = RPoint_x(surface_points[point]);
        parameters[IJ(point,Y,3)] = RPoint_y(surface_points[point]);
        parameters[IJ(point,Z,3)] = RPoint_z(surface_points[point]);
    }

    iter = 0;
    while( iter < n_iters )
    {
        create_image_coefficients( volume, &boundary, tangent_weight,
                                   max_outward, max_inward, floating_flag,
                                   oversample,
                                   n_points, n_neighbours, neighbours,
                                   parameters,
                                   &parm_list[n_model_equations],
                                   &constants[n_model_equations],
                                   &node_weights[n_model_equations] );

#ifdef DEBUG
#define DEBUG
#undef DEBUG
    {
    int  e;
    for_less( e, 0, n_image_equations )
    {
        int  p;
        eq = e + n_model_equations;
        print( "%3d: %g : ", eq, constants[eq] );
        for_less( p, 0, n_parms_involved[eq] )
        {
            print( " %d:%g ", parm_list[eq][p], node_weights[eq][p] );
        }
        print( "\n" );
    }
    }
#endif

#ifdef USING_FLOAT
#define  MINIMIZE_LSQ  minimize_lsq_float
#else
#define  MINIMIZE_LSQ  minimize_lsq
#endif

        (void) MINIMIZE_LSQ( 3 * n_points, n_equations,
                             n_parms_involved, parm_list, constants,
                             node_weights, n_iters_recompute, parameters );

        iter += n_iters_recompute;
        print( "########### %d:\n", iter );
        (void) flush_file( stdout );
    }

    for_less( point, 0, n_points )
    {
        fill_Point( surface_points[point],
                    parameters[IJ(point,X,3)],
                    parameters[IJ(point,Y,3)],
                    parameters[IJ(point,Z,3)] )
    }

    save_p.n_points = n_points;
    delete_polygon_point_neighbours( &save_p, n_neighbours,
                                     neighbours, NULL, NULL );

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
}
