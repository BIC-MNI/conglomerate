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

private  void  fit_polygons(
    polygons_struct    *surface,
    polygons_struct    *model_surface,
    Real               model_weight,
    Volume             volume,
    Real               threshold,
    char               normal_direction,
    Real               tangent_weight,
    Real               max_outward,
    Real               max_inward,
    int                n_iters,
    int                n_iters_recompute );

private  void  usage(
    char   executable_name[] )
{
    STRING  usage_format = "\
Usage:     %s  input.obj output.obj model.obj model_weight volume.mnc \n\
                  threshold +|-|0  tangent_weight out_dist in_dist\n\
                  [n_iters] [n_between]\n\n";

    print_error( usage_format, executable_name );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               input_filename, output_filename, model_filename;
    STRING               surface_direction, volume_filename;
    int                  n_objects, n_m_objects;
    int                  n_iters, n_iters_recompute;
    File_formats         format;
    object_struct        **object_list, **m_object_list;
    polygons_struct      *surface, *model_surface;
    Volume               volume;
    Real                 threshold, model_weight;
    Real                 max_outward, max_inward, tangent_weight;

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
        !get_real_argument( 0.0, &max_inward ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_iters );
    (void) get_int_argument( 1, &n_iters_recompute );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    surface = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( model_filename, &format, &n_m_objects,
                             &m_object_list ) != OK || n_m_objects != 1 ||
        get_object_type(m_object_list[0]) != POLYGONS )
        return( 1 );

    model_surface = get_polygons_ptr( m_object_list[0] );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    fit_polygons( surface, model_surface, model_weight, volume, threshold,
                  surface_direction[0], tangent_weight, max_outward, max_inward,
                  n_iters, n_iters_recompute );

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
    polygons_struct  *model_surface,
    int              n_neighbours[],
    int              *neighbours[],
    int              n_parms_involved[],
    int              *parm_list[],
    ftype            constants[],
    ftype            *node_weights[] )
{
    int              node, eq, dim, dim1, n_nodes, n, ind;
    FILE             *file;
    Real             *x_flat;
    Real             *y_flat;
    Real             *z_flat;
    int              *neigh_indices, n_nn, max_neighbours;
    BOOLEAN          ignoring;
    Real             *weights[N_DIMENSIONS][N_DIMENSIONS];
    progress_struct  progress;

    n_nodes = model_surface->n_points;

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
            x_flat[n] = RPoint_x( model_surface->points[neigh_indices[n]] );
            y_flat[n] = RPoint_y( model_surface->points[neigh_indices[n]] );
            z_flat[n] = RPoint_z( model_surface->points[neigh_indices[n]] );
        }

        ignoring = FALSE;
        if( !get_prediction_weights_3d( RPoint_x(model_surface->points[node]),
                                        RPoint_y(model_surface->points[node]),
                                        RPoint_z(model_surface->points[node]),
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

private  void  create_image_coefficients(
    Volume                      volume,
    boundary_definition_struct  *boundary,
    Real                        tangent_weight,
    Real                        max_outward,
    Real                        max_inward,
    int                         n_nodes,
    int                         n_neighbours[],
    int                         *neighbours[],
    Real                        parameters[],
    int                         *parm_list[],
    ftype                       constants[],
    ftype                       *node_weights[] )
{
    int     eq, node, n, n_to_do;
    Real    dist, dx, dy, dz, value;
    Point   origin, p;
    Point   neigh_points[100];
    Vector  normal, vert, hor;

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
            dist = MAX( max_outward, max_inward );
            if( tangent_weight == 0.0 )
                n_to_do = 1;
            else
                n_to_do = 3;

            for_less( n, 0, n_to_do )
            {
                node_weights[eq][0] = (ftype) 0.0;
                node_weights[eq][1] = (ftype) 0.0;
                node_weights[eq][2] = (ftype) 0.0;
                constants[eq] = (ftype) (dist * dist);
                ++eq;
            }

            continue;
        }

        GET_POINT_ON_RAY( p, origin, normal, dist );

        evaluate_volume_in_world( volume, RPoint_x(p), RPoint_y(p), RPoint_z(p),
                                  0, FALSE, 0.0, &value,
                                  &dx, &dy, &dz,
                                  NULL, NULL, NULL, NULL, NULL, NULL );

        fill_Vector( normal, dx, dy, dz );
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

            node_weights[eq][0] = (ftype) (tangent_weight * RVector_x(vert));
            node_weights[eq][1] = (ftype) (tangent_weight * RVector_y(vert));
            node_weights[eq][2] = (ftype) (tangent_weight * RVector_z(vert));
            constants[eq] = (ftype) (-tangent_weight *
                                     DOT_POINT_VECTOR( p, vert ));
            ++eq;
        }
    }
}

private  void  fit_polygons(
    polygons_struct    *surface,
    polygons_struct    *model_surface,
    Real               model_weight,
    Volume             volume,
    Real               threshold,
    char               normal_direction,
    Real               tangent_weight,
    Real               max_outward,
    Real               max_inward,
    int                n_iters,
    int                n_iters_recompute )
{
    int              eq, point, n, iter, n_points, which;
    int              n_model_equations, n_image_equations;
    int              n_equations, *n_parms_involved, **parm_list;
    int              *n_neighbours, **neighbours, n_image_per_point;
    ftype            *constants, **node_weights;
    Real             *parameters;
    boundary_definition_struct  boundary;

    n_points = surface->n_points;

    create_polygon_point_neighbours( surface, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    set_boundary_definition( &boundary, threshold, threshold, -1.0, 90.0,
                             normal_direction, 1.0e-4 );

    n_model_equations = 3 * n_points;

    if( tangent_weight > 0.0 )
        n_image_per_point = 3;
    else
        n_image_per_point = 1;

    n_image_equations = n_image_per_point * n_points;

    n_equations = n_model_equations + n_image_equations;

    ALLOC( n_parms_involved, n_equations );
    ALLOC( constants, n_equations );
    ALLOC( node_weights, n_equations );
    ALLOC( parm_list, n_equations );

    create_model_coefficients( model_surface, n_neighbours, neighbours,
                               n_parms_involved,
                               parm_list, constants, node_weights );

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
            n_parms_involved[eq] = 3;
            ALLOC( node_weights[eq], 3 );
            ALLOC( parm_list[eq], 3 );
            parm_list[eq][0] = IJ(point,X,3);
            parm_list[eq][1] = IJ(point,Y,3);
            parm_list[eq][2] = IJ(point,Z,3);
        }
    }

    ALLOC( parameters, 3 * n_points );

    for_less( point, 0, n_points )
    {
        parameters[IJ(point,X,3)] = RPoint_x(surface->points[point]);
        parameters[IJ(point,Y,3)] = RPoint_y(surface->points[point]);
        parameters[IJ(point,Z,3)] = RPoint_z(surface->points[point]);
    }

    iter = 0;
    while( iter < n_iters )
    {
        create_image_coefficients( volume, &boundary, tangent_weight,
                                   max_outward, max_inward,
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
        fill_Point( surface->points[point],
                    parameters[IJ(point,X,3)],
                    parameters[IJ(point,Y,3)],
                    parameters[IJ(point,Z,3)] )
    }

    delete_polygon_point_neighbours( surface, n_neighbours,
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
