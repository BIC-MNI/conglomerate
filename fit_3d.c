#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <deform.h>

#undef   USING_FLOAT
#define  USING_FLOAT

#ifdef USING_FLOAT

typedef  float  ftype;
#define  MINIMIZE_LSQ  minimize_lsq_float

#else

typedef  Real   ftype;
#define  MINIMIZE_LSQ  minimize_lsq

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
    Smallest_int       fit_this_node[],
    BOOLEAN            floating_flag,
    int                oversample,
    Real               max_step,
    int                n_iters,
    int                n_iters_recompute );

private  void  usage(
    char   executable_name[] )
{
    STRING  usage_format = "\
Usage:     %s  input.obj output.obj model.obj model_weight volume.mnc \n\
                  threshold +|-|0  tangent_weight out_dist in_dist\n\
                  float/nofloat oversample\n\
                  [n_iters] [n_between] [max_step]\n\
                  [values_file min max]\n\n";

    print_error( usage_format, executable_name );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               input_filename, output_filename, model_filename;
    STRING               surface_direction, volume_filename, floating_string;
    STRING               values_filename;
    FILE                 *file;
    int                  i, n_objects, n_m_objects;
    int                  n_iters, n_iters_recompute, n_points;
    int                  *n_neighbours, **neighbours, oversample;
    File_formats         format;
    object_struct        **object_list, **m_object_list;
    polygons_struct      *surface, *model_surface;
    Volume               volume;
    Point                *surface_points, *model_points;
    Real                 threshold, model_weight;
    Real                 max_outward, max_inward, tangent_weight;
    Smallest_int         *fit_this_node;
    Real                 min_value, max_value, value, max_step;
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
    (void) get_real_argument( -1.0, &max_step );
    (void) get_string_argument( NULL, &values_filename );
    (void) get_real_argument( 0.0, &min_value );
    (void) get_real_argument( 0.0, &max_value );

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

    if( values_filename != NULL )
    {
        ALLOC( fit_this_node, n_points );
        if( open_file( values_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );

        for_less( i, 0, n_points )
        {
            if( input_real( file, &value ) != OK )
            {
                print_error( "Error reading values.\n" );
                return( 1 );
            }

            fit_this_node[i] = (Smallest_int)
                            (min_value <= value && value <= max_value);
        }

        (void) close_file( file );
    }
    else
        fit_this_node = NULL;

    fit_polygons( n_points, n_neighbours, neighbours, surface_points,
                  model_points, model_weight, volume, threshold,
                  surface_direction[0], tangent_weight, max_outward, max_inward,
                  fit_this_node,
                  floating_flag, oversample, max_step,
                  n_iters, n_iters_recompute );

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

private  void  display_parms(
    int      n_equations,
    int      n_parms_involved[],
    int      *parm_list[],
    ftype    constants[],
    ftype    *node_weights[],
    Real     parameters[] )
{
    int   eq, p;
    Real  value;

    for_less( eq, 0, n_equations )
    {
        value = (Real) constants[eq];
        print( "%3d: %g : ", eq, constants[eq] );
        for_less( p, 0, n_parms_involved[eq] )
        {
            print( " %d:%g ", parm_list[eq][p], node_weights[eq][p] );
            value += (Real) node_weights[eq][p] * parameters[parm_list[eq][p]];
        }
        print( " : %g\n", value );
    }
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
    int              to_parameter[],
    Point            surface_points[],
    Point            model_points[],
    int              n_neighbours[],
    int              *neighbours[],
    int              n_parms_involved[],
    int              *parm_list[],
    ftype            constants[],
    ftype            *node_weights[] )
{
    int              node, eq, dim, dim1, n, ind;
    Real             *x_flat, *y_flat, *z_flat, con;
    int              *neigh_indices, n_nn, max_neighbours;
    BOOLEAN          ignoring;
    Real             *weights[N_DIMENSIONS][N_DIMENSIONS];
    progress_struct  progress;

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

    for_less( dim, 0, N_DIMENSIONS )
    for_less( dim1, 0, N_DIMENSIONS )
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

        for_less( dim, 0, N_DIMENSIONS )
        {
            ind = 1;
            for_less( n, 0, n_nn )
            for_less( dim1, 0, N_DIMENSIONS )
            {
                if( weights[dim][dim1][n] != 0.0 &&
                    to_parameter[neigh_indices[n]] >= 0 )
                    ++ind;
            }

            n_parms_involved[eq] = ind;

            ALLOC( parm_list[eq], n_parms_involved[eq] );
            ALLOC( node_weights[eq], n_parms_involved[eq] );

            ind = 0;
            parm_list[eq][ind] = IJ(to_parameter[node],dim,3);
            node_weights[eq][ind] = (ftype) 1.0;
            ++ind;

            con = 0.0;
            for_less( n, 0, n_nn )
            for_less( dim1, 0, N_DIMENSIONS )
            {
                if( weights[dim][dim1][n] != 0.0 )
                {
                    if( to_parameter[neigh_indices[n]] >= 0 )
                    {
                        parm_list[eq][ind] = IJ(to_parameter[neigh_indices[n]],
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
    voxel_coef_struct           *voxel_lookup,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    boundary_definition_struct  *boundary,
    Real                        tangent_weight,
    Real                        max_outward,
    Real                        max_inward,
    BOOLEAN                     floating_flag,
    int                         oversample,
    int                         n_nodes,
    int                         n_neighbours[],
    int                         *neighbours[],
    int                         to_parameter[],
    Real                        parameters[],
    Point                       surface_points[],
    int                         *parm_list[],
    ftype                       constants[],
    ftype                       *node_weights[] )
{
    int        eq, node, n, n_to_do, neigh, n2, neigh2, w, parm_index;
    int        i, n_involved, inv_index, neigh_parm_index, d;
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
        if( to_parameter[node] < 0 )
            continue;

        for_less( n, 0, n_neighbours[node] )
        {
            neigh = neighbours[node][n];
            neigh_parm_index = to_parameter[neigh];
            if( neigh_parm_index >= 0 )
            {
                fill_Point( neigh_points[n],
                            parameters[IJ(neigh_parm_index,X,3)],
                            parameters[IJ(neigh_parm_index,Y,3)],
                            parameters[IJ(neigh_parm_index,Z,3)] );
            }
            else
                neigh_points[n] = surface_points[neigh];
        }

        find_polygon_normal( n_neighbours[node], neigh_points, &normal );

        parm_index = to_parameter[node];
        if( parm_index >= 0 )
        {
            fill_Point( origin,
                        parameters[IJ(parm_index,X,3)],
                        parameters[IJ(parm_index,Y,3)],
                        parameters[IJ(parm_index,Z,3)] );
        }
        else
            origin = surface_points[node];

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

                for_less( n, 0, n_to_do )
                {
                    if( tangent_weight == 1.0 )
                    {
                        node_weights[eq][0] = (ftype) 0.0;
                    }
                    else
                    {
                        node_weights[eq][0] = (ftype) 0.0;
                        node_weights[eq][1] = (ftype) 0.0;
                        node_weights[eq][2] = (ftype) 0.0;
                    }

                    constants[eq] = (ftype) dist;
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

#ifdef DEBUG
#define DEBUG
#undef DEBUG
{
    int  e, n_parms_involved;
    if( tangent_weight == 1.0 )
        n_parms_involved = 1;
    else
        n_parms_involved = 3;
    for_less( e, 0, eq )
    {
        int  p;
        print( "%3d: %g : ", e, constants[e] );
        for_less( p, 0, n_parms_involved )
        {
            print( " %d:%g ", parm_list[e][p], node_weights[e][p] );
        }
        print( "\n" );
    }
}
#endif

    if( oversample <= 0 )
        return;

    for_less( node, 0, n_nodes )
    {
        for_less( n, 0, n_neighbours[node] )
        {
            neigh = neighbours[node][n];
            neigh_parm_index = to_parameter[neigh];
            if( neigh_parm_index >= 0 )
            {
                fill_Point( neigh_points[n],
                            parameters[IJ(neigh_parm_index,X,3)],
                            parameters[IJ(neigh_parm_index,Y,3)],
                            parameters[IJ(neigh_parm_index,Z,3)] );
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
                                parameters[IJ(neigh_parm_index,X,3)],
                                parameters[IJ(neigh_parm_index,Y,3)],
                                parameters[IJ(neigh_parm_index,Z,3)] );
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
                angle = 2.0 * PI - compute_clockwise_rotation( x, y );
                if( angle < 0.0 || angle > PI )
                    handle_internal_error( "angle < 0.0 || 180.0" );
            }

            n_involved = 0;
            parm_index = to_parameter[node];
            if( parm_index >= 0 )
            {
                fill_Point( p1,
                            parameters[IJ(parm_index,X,3)],
                            parameters[IJ(parm_index,Y,3)],
                            parameters[IJ(parm_index,Z,3)] );
                ++n_involved;
            }
            else
                p1 = surface_points[node];

            neigh_parm_index = to_parameter[neigh];
            if( neigh_parm_index >= 0 )
            {
                fill_Point( p2,
                            parameters[IJ(neigh_parm_index,X,3)],
                            parameters[IJ(neigh_parm_index,Y,3)],
                            parameters[IJ(neigh_parm_index,Z,3)] );
                ++n_involved;
            }
            else
                p2 = surface_points[neigh];

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

                        for_less( d, 0, n_to_do )
                        {
                            if( tangent_weight == 1.0 )
                            {
                                for_less( i, 0, n_involved )
                                    node_weights[eq][i] = (ftype) 0.0;
                            }
                            else
                            {
                                for_less( i, 0, 3 * n_involved )
                                    node_weights[eq][i] = (ftype) 0.0;
                            }

                            constants[eq] = (ftype) dist;
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
                    constants[eq+0]       = (ftype) -RPoint_x(p);
                    constants[eq+1]       = (ftype) -RPoint_y(p);
                    constants[eq+2]       = (ftype) -RPoint_z(p);

                    inv_index = 0;
                    if( parm_index >= 0 )
                    {
                        node_weights[eq+0][0] = (ftype) (1.0 - alpha);
                        node_weights[eq+1][0] = (ftype) (1.0 - alpha);
                        node_weights[eq+2][0] = (ftype) (1.0 - alpha);
                        ++inv_index;
                    }
                    else
                    {
                        constants[eq+0] += (ftype) ((1.0-alpha) * RPoint_x(p1));
                        constants[eq+1] += (ftype) ((1.0-alpha) * RPoint_y(p1));
                        constants[eq+2] += (ftype) ((1.0-alpha) * RPoint_z(p1));
                    }

                    if( neigh_parm_index >= 0 )
                    {
                        node_weights[eq+0][inv_index] = (ftype) alpha;
                        node_weights[eq+1][inv_index] = (ftype) alpha;
                        node_weights[eq+2][inv_index] = (ftype) alpha;
                    }
                    else
                    {
                        constants[eq+0] += (ftype) (alpha * RPoint_x(p2));
                        constants[eq+1] += (ftype) (alpha * RPoint_y(p2));
                        constants[eq+2] += (ftype) (alpha * RPoint_z(p2));
                    }

                    eq += 3;
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
        
                    inv_index = 0;
                    constants[eq] = (ftype) -DOT_POINT_VECTOR( p, normal );

                    if( parm_index >= 0 )
                    {
                        node_weights[eq][0] = (ftype)((1.0 - alpha) *
                                              RVector_x(normal));
                        node_weights[eq][1] = (ftype)((1.0 - alpha) *
                                              RVector_y(normal));
                        node_weights[eq][2] = (ftype)((1.0 - alpha) *
                                              RVector_z(normal));
                        ++inv_index;
                    }
                    else
                    {
                        constants[eq] += (ftype) ((1.0 - alpha) *
                                           DOT_POINT_VECTOR( p1, normal ));
                    }

                    if( neigh_parm_index >= 0 )
                    {
                        node_weights[eq][3*inv_index+0] = (ftype)( alpha  *
                                                          RVector_x(normal));
                        node_weights[eq][3*inv_index+1] = (ftype)( alpha  *
                                                          RVector_y(normal));
                        node_weights[eq][3*inv_index+2] = (ftype)( alpha  *
                                                          RVector_z(normal));
                    }
                    else
                    {
                        constants[eq] += (ftype) (alpha *
                                               DOT_POINT_VECTOR( p2, normal ));
                    }

                    ++eq;

                    if( tangent_weight <= 0.0 )
                        continue;

                    create_two_orthogonal_vectors( &normal, &hor, &vert );
                    NORMALIZE_VECTOR( hor, hor );
                    NORMALIZE_VECTOR( vert, vert );

                    inv_index = 0;
                    constants[eq] = (ftype) (-tangent_weight *
                                             DOT_POINT_VECTOR( p, hor ));
                    constants[eq+1] = (ftype) (-tangent_weight *
                                             DOT_POINT_VECTOR( p, vert ));

                    if( parm_index >= 0 )
                    {
                        node_weights[eq+0][0] =
                                     (ftype) ((1.0 - alpha) * tangent_weight *
                                              RVector_x(hor));
                        node_weights[eq+0][1] =
                                     (ftype) ((1.0 - alpha) * tangent_weight *
                                              RVector_y(hor));
                        node_weights[eq+0][2] =
                                     (ftype) ((1.0 - alpha) * tangent_weight *
                                              RVector_z(hor));

                        node_weights[eq+1][0] = (ftype) ((1.0 - alpha) *
                                              tangent_weight * RVector_x(vert));
                        node_weights[eq+1][1] = (ftype) ((1.0 - alpha) *
                                              tangent_weight * RVector_y(vert));
                        node_weights[eq+1][2] = (ftype) ((1.0 - alpha) *
                                              tangent_weight * RVector_z(vert));
                        ++inv_index;
                    }
                    else
                    {
                        constants[eq] += (ftype) ((1.0 - alpha) *
                                 tangent_weight * DOT_POINT_VECTOR( p1, hor ));
                        constants[eq+1] += (ftype) ((1.0 - alpha) *
                                 tangent_weight * DOT_POINT_VECTOR( p1, vert ));
                    }

                    if( neigh_parm_index >= 0 )
                    {
                        node_weights[eq][3*inv_index+0] = (ftype) (alpha *
                                              tangent_weight * RVector_x(hor));
                        node_weights[eq][3*inv_index+1] = (ftype) (alpha *
                                              tangent_weight * RVector_y(hor));
                        node_weights[eq][3*inv_index+2] = (ftype) (alpha *
                                              tangent_weight * RVector_z(hor));

                        node_weights[eq+1][3*inv_index+0] = (ftype) ( alpha *
                                              tangent_weight * RVector_x(vert));
                        node_weights[eq+1][3*inv_index+1] = (ftype) ( alpha *
                                              tangent_weight * RVector_y(vert));
                        node_weights[eq+1][3*inv_index+2] = (ftype) ( alpha *
                                              tangent_weight * RVector_z(vert));
                    }
                    else
                    {
                        constants[eq] += (ftype) (alpha * tangent_weight *
                                                 DOT_POINT_VECTOR( p2, hor ));
                        constants[eq+1] += (ftype) (alpha * tangent_weight *
                                                 DOT_POINT_VECTOR( p2, vert ));
                    }

                    eq += 2;
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
    Smallest_int       fit_this_node[],
    BOOLEAN            floating_flag,
    int                oversample,
    Real               max_step,
    int                n_iters,
    int                n_iters_recompute )
{
    int                         eq, point, n, iter, which, o;
    int                         n_model_equations, n_image_equations;
    int                         n_equations, *n_parms_involved, **parm_list;
    int                         n_image_per_point, n_involved;
    int                         n_oversample_equations, n_bound_nodes;
    int                         n_moving_points;
    int                         sizes[N_DIMENSIONS];
    int                         *to_parameter;
    int                         parm_index, neigh_parm_index;
    ftype                       *constants, **node_weights, weight;
    Real                        *parameters;
    polygons_struct             save_p;
    boundary_definition_struct  boundary;
    voxel_coef_struct           voxel_lookup;
    bitlist_3d_struct           done_bits, surface_bits;
    bitlist_3d_struct           *done_bits_ptr, *surface_bits_ptr;


    set_boundary_definition( &boundary, threshold, threshold, -1.0, 90.0,
                             normal_direction, 1.0e-4 );

    if( tangent_weight > 0.0 )
        n_image_per_point = 3;
    else
        n_image_per_point = 1;

    if( tangent_weight == 1.0 )
        n_bound_nodes = 1;
    else
        n_bound_nodes = 3;

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

    create_model_coefficients( n_points, to_parameter, surface_points,
                               model_points, n_neighbours, neighbours,
                               n_parms_involved,
                               parm_list, constants, node_weights );

    FREE( model_points );

    model_weight = sqrt( model_weight / (Real) n_model_equations );

    for_less( eq, 0, n_model_equations )
    {
        constants[eq] *= (ftype) model_weight;
        for_less( n, 0, n_parms_involved[eq] )
            node_weights[eq][n] *= (ftype) model_weight;
    }

    eq = n_model_equations;
    for_less( point, 0, n_points )
    {
        if( to_parameter[point] < 0 )
            continue;

        for_less( which, 0, n_image_per_point )
        {
            n_parms_involved[eq] = n_bound_nodes;
            ALLOC( node_weights[eq], n_parms_involved[eq] );
            ALLOC( parm_list[eq], n_parms_involved[eq] );
            parm_index = to_parameter[point];
            if( n_bound_nodes == 1 )
                parm_list[eq][0] = IJ(parm_index,which,3);
            else
            {
                parm_list[eq][0] = IJ(parm_index,X,3);
                parm_list[eq][1] = IJ(parm_index,Y,3);
                parm_list[eq][2] = IJ(parm_index,Z,3);
            }
            ++eq;
        }
    }

    eq = n_model_equations + n_image_equations;
    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            if( oversample > 0 &&
                this_is_unique_edge( point, neighbours[point][n],
                                     n_neighbours, neighbours ) &&
                (to_parameter[point] >= 0 ||
                 to_parameter[neighbours[point][n]] >= 0) )
            {
                for_less( o, 0, oversample )
                for_less( which, 0, n_image_per_point )
                {
                    n_involved = 0;
                    parm_index = to_parameter[point];
                    neigh_parm_index = to_parameter[neighbours[point][n]];
                    if( parm_index >= 0 )
                        ++n_involved;
                    if( neigh_parm_index >= 0 )
                        ++n_involved;

                    n_parms_involved[eq] = n_involved * n_bound_nodes;
                    ALLOC( node_weights[eq], n_parms_involved[eq] );
                    ALLOC( parm_list[eq], n_parms_involved[eq] );

                    n_involved = 0;
                    if( n_bound_nodes == 1 )
                    {
                        if( parm_index >= 0 )
                            parm_list[eq][n_involved++] =IJ(parm_index,which,3);
                        if( neigh_parm_index >= 0 )
                        {
                            parm_list[eq][n_involved] = 
                                          IJ(neigh_parm_index,which,3);
                        }
                    }
                    else
                    {
                        if( parm_index >= 0 )
                        {
                            parm_list[eq][n_involved++] = IJ(parm_index,X,3);
                            parm_list[eq][n_involved++] = IJ(parm_index,Y,3);
                            parm_list[eq][n_involved++] = IJ(parm_index,Z,3);
                        }

                        if( neigh_parm_index >= 0 )
                        {
                            parm_list[eq][n_involved++] =
                                                    IJ(neigh_parm_index,X,3);
                            parm_list[eq][n_involved++] =
                                                    IJ(neigh_parm_index,Y,3);
                            parm_list[eq][n_involved++] =
                                                    IJ(neigh_parm_index,Z,3);
                        }
                    }

                    ++eq;
                }
            }
        }
    }

    ALLOC( parameters, 3 * n_moving_points );

    for_less( point, 0, n_points )
    {
        parm_index = to_parameter[point];
        if( parm_index >= 0 )
        {
            parameters[IJ(parm_index,X,3)] = RPoint_x(surface_points[point]);
            parameters[IJ(parm_index,Y,3)] = RPoint_y(surface_points[point]);
            parameters[IJ(parm_index,Z,3)] = RPoint_z(surface_points[point]);
        }
    }

    initialize_lookup_volume_coeficients( &voxel_lookup );
    get_volume_sizes( volume, sizes );

    done_bits_ptr = &done_bits;
    surface_bits_ptr = &surface_bits;

    if( done_bits_ptr != NULL )
    {
        create_bitlist_3d( sizes[X], sizes[Y], sizes[Z], &done_bits );
        create_bitlist_3d( sizes[X], sizes[Y], sizes[Z], &surface_bits );
    }

    iter = 0;
    while( iter < n_iters )
    {
        create_image_coefficients( volume, &voxel_lookup,
                                   done_bits_ptr, surface_bits_ptr,
                                   &boundary, tangent_weight,
                                   max_outward, max_inward,
                                   floating_flag, oversample,
                                   n_points, n_neighbours, neighbours,
                                   to_parameter, parameters, surface_points,
                                   &parm_list[n_model_equations],
                                   &constants[n_model_equations],
                                   &node_weights[n_model_equations] );

    weight = (ftype)
              sqrt( 1.0 / (Real) (n_image_equations + n_oversample_equations) );

    for_less( eq, n_model_equations, n_equations )
    {
        constants[eq] *= (ftype) weight;
        for_less( n, 0, n_parms_involved[eq] )
            node_weights[eq][n] *= weight;
    }


#ifdef DEBUG
#define DEBUG
#undef DEBUG
    display_parms( n_equations, n_parms_involved, parm_list, constants,
                   node_weights, parameters );
#endif

        (void) MINIMIZE_LSQ( 3 * n_moving_points, n_equations,
                             n_parms_involved, parm_list, constants,
                             node_weights, max_step, n_iters_recompute,
                             parameters );

        iter += n_iters_recompute;
        print( "########### %d:\n", iter );
        (void) flush_file( stdout );
    }

    for_less( point, 0, n_points )
    {
        parm_index = to_parameter[point];
        if( parm_index >= 0 )
        {
            fill_Point( surface_points[point],
                        parameters[IJ(parm_index,X,3)],
                        parameters[IJ(parm_index,Y,3)],
                        parameters[IJ(parm_index,Z,3)] )
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
}
