#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename, initial_filename;
    int                  n_objects, n_i_objects, n_iters;
    File_formats         format;
    object_struct        **object_list, **i_object_list;
    polygons_struct      *polygons;
    Point                *init_points;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj [n_iters]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_iters );

    if( get_string_argument( NULL, &initial_filename ) )
    {
        if( input_graphics_file( initial_filename, &format, &n_i_objects,
                                 &i_object_list ) != OK || n_i_objects != 1 ||
            get_object_type(i_object_list[0]) != POLYGONS )
            return( 1 );

        polygons = get_polygons_ptr( i_object_list[0] );
        init_points = polygons->points;
        ALLOC( polygons->points, 1 );
        delete_object_list( n_i_objects, i_object_list );
    }
    else
    {
        init_points = NULL;
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    flatten_polygons( polygons, init_points, n_iters );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

private  int  create_coefficients(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              **neighbours,
    Smallest_int     interior_flags[],
    int              n_fixed,
    int              fixed_indices[],
    Real             *fixed_pos[2],
    int              to_parameters[],
    int              to_fixed_index[],
    int              *n_nodes_involved[],
    int              **node_list[],
    Real             *constants[],
    Real             **node_weights[] )
{
    int              n_equations, node, p, eq, dim, dim1, n_nodes_in;
    int              neigh, max_neighbours;
    Point            neigh_points[MAX_POINTS_PER_POLYGON];
    Real             flat[3][MAX_POINTS_PER_POLYGON];
    Real             *weights[3][3];
    BOOLEAN          found, ignoring;
    progress_struct  progress;

    max_neighbours = 0;
    for_less( node, 0, polygons->n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[node] );

    for_less( dim, 0, N_DIMENSIONS )
    for_less( dim1, 0, N_DIMENSIONS )
       ALLOC( weights[dim][dim1], max_neighbours );

    n_equations = 0;
    for_less( node, 0, polygons->n_points )
    {
        found = to_parameters[node] >= 0;
        for_less( p, 0, n_neighbours[node] )
        {
            if( to_parameters[neighbours[node][p]] >= 0 )
            {
                found = TRUE;
                break;
            }
        }
        if( found )
            ++n_equations;
    }

    n_equations *= 3;

    ALLOC( *n_nodes_involved, n_equations );
    ALLOC( *constants, n_equations );
    ALLOC( *node_weights, n_equations );
    ALLOC( *node_list, n_equations );

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Creating coefficients" );
    eq = 0;
    for_less( node, 0, polygons->n_points )
    {
        found = to_parameters[node] >= 0;
        for_less( p, 0, n_neighbours[node] )
        {
            if( to_parameters[neighbours[node][p]] >= 0 )
            {
                found = TRUE;
                break;
            }
        }

        if( !found )
            continue;

        for_less( p, 0, n_neighbours[node] )
            neigh_points[p] = polygons->points[neighbours[node][p]];

        flatten_around_vertex( &polygons->points[node],
                               n_neighbours[node], neigh_points,
                               (BOOLEAN) interior_flags[node],
                               flat[0], flat[1] );

        for_less( p, 0, n_neighbours[node] )
            flat[2][p] = 0.0;

        ignoring = FALSE;
        if( !get_prediction_weights_3d( 0.0, 0.0, 0.0,
                                        n_neighbours[node],
                                        flat[0], flat[1], flat[2],
                                        weights[0], weights[1], weights[2] ) )
        {
            print_error( "Error in interpolation weights, ignoring..\n" );
            ignoring = TRUE;
        }

        if( ignoring )
        {
            (*n_nodes_involved)[eq] = 0;
            (*constants)[eq] = 0.0;
            ++eq;
            (*n_nodes_involved)[eq] = 0;
            (*constants)[eq] = 0.0;
            ++eq;
            (*n_nodes_involved)[eq] = 0;
            (*constants)[eq] = 0.0;
            ++eq;
            continue;
        }

        for_less( dim, 0, 3 )
        {
            n_nodes_in = 0;
            if( to_parameters[node] >= 0 )
                ++n_nodes_in;

            for_less( p, 0, n_neighbours[node] )
            {
                if( to_parameters[neighbours[node][p]] >= 0 )
                    ++n_nodes_in;
            }

            (*n_nodes_involved)[eq] = n_nodes_in;
            ALLOC( (*node_list)[eq], (*n_nodes_involved)[eq] );
            ALLOC( (*node_weights)[eq], (*n_nodes_involved)[eq] );

            n_nodes_in = 0;
            if( to_parameters[node] >= 0 )
            {
                (*node_list)[eq][0] = IJ( to_parameters[node],dim,3);
                (*node_weights)[eq][0] = 1.0;
                ++n_nodes_in;
                (*constants)[eq] = 0.0;
            }
            else
                (*constants)[eq] = fixed_pos[dim][to_fixed_index[node]];

            for_less( p, 0, n_neighbours[node] )
            {
                neigh = neighbours[node][p];
                if( to_parameters[neigh] >= 0 )
                {
                    (*node_list)[eq][n_nodes_in] =
                                         IJ(to_parameters[neigh],dim,3);
                    (*node_weights)[eq][n_nodes_in] = -weights[dim][dim][p];
                    ++n_nodes_in;
                }
                else
                {
                    (*constants)[eq] += -weights[dim][dim][p] *
                                        fixed_pos[dim][to_fixed_index[neigh]];
                }
            }

            ++eq;
        }

        update_progress_report( &progress, node + 1 );
    }

    terminate_progress_report( &progress );

    for_less( dim, 0, N_DIMENSIONS )
    for_less( dim1, 0, N_DIMENSIONS )
       FREE( weights[dim][dim1] );

    return( n_equations );
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters )
{
    int              i, point, *n_neighbours, **neighbours;
    int              n_equations, *n_nodes_per_equation, **node_list;
    int              n_fixed, *fixed_indices, size;
    Real             *fixed_pos[3];
    Real             *constants, **node_weights;
    Point            *new_points;
    Real             *parameters;
    int              *to_parameters, *to_fixed_index, ind;
    Smallest_int     *interior_flags;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, &interior_flags, NULL );

    size = GET_OBJECT_SIZE( *polygons, 0 );

    n_fixed = size;
    ALLOC( fixed_indices, n_fixed );
    ALLOC( fixed_pos[0], n_fixed );
    ALLOC( fixed_pos[1], n_fixed );
    ALLOC( fixed_pos[2], n_fixed );

    for_less( i, 0, size )
    {
        fixed_indices[i] = polygons->indices[
                            POINT_INDEX(polygons->end_indices,0,i)];
        fixed_pos[0][i] = RPoint_x(polygons->points[fixed_indices[i]] );
        fixed_pos[1][i] = RPoint_y(polygons->points[fixed_indices[i]] );
        fixed_pos[2][i] = RPoint_z(polygons->points[fixed_indices[i]] );
    }

    ALLOC( to_parameters, polygons->n_points );
    ALLOC( to_fixed_index, polygons->n_points );
    ind = 0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( i, 0, n_fixed )
        {
            if( point == fixed_indices[i] )
                break;
        }
        if( i < n_fixed )
        {
            to_fixed_index[point] = i;
            to_parameters[point] = -1;
        }
        else
        {
            to_fixed_index[point] = -1;
            to_parameters[point] = ind;
            ++ind;
        }
    }


    n_equations = create_coefficients( polygons,
                                       n_neighbours, neighbours, interior_flags,
                                       n_fixed, fixed_indices, fixed_pos,
                                       to_parameters, to_fixed_index,
                                       &n_nodes_per_equation,
                                       &node_list, &constants, &node_weights );
    ALLOC( parameters, 3 * (polygons->n_points - n_fixed) );

    if( init_points == NULL )
        init_points = polygons->points;

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            parameters[IJ(to_parameters[point],X,3)] =
                           RPoint_x(init_points[point]);
            parameters[IJ(to_parameters[point],Y,3)] =
                           RPoint_y(init_points[point]);
            parameters[IJ(to_parameters[point],Z,3)] =
                           RPoint_z(init_points[point]);
        }
    }

    (void) minimize_lsq( 3 * (polygons->n_points - n_fixed), n_equations,
                         n_nodes_per_equation, node_list, constants,
                         node_weights, n_iters, parameters );

    ALLOC( new_points, polygons->n_points );

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            fill_Point( new_points[point],
                        parameters[IJ(to_parameters[point],X,3)],
                        parameters[IJ(to_parameters[point],Y,3)],
                        parameters[IJ(to_parameters[point],Z,3)] );
        }
        else
        {
            fill_Point( new_points[point],
                        fixed_pos[0][to_fixed_index[point]],
                        fixed_pos[1][to_fixed_index[point]],
                        fixed_pos[2][to_fixed_index[point]] );
        }
    }

    FREE( parameters );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     interior_flags, NULL );

    for_less( point, 0, polygons->n_points )
        polygons->points[point] = new_points[point];

    FREE( new_points );
    FREE( to_parameters );
    FREE( to_fixed_index );
    FREE( fixed_pos[0] );
    FREE( fixed_pos[1] );
}
