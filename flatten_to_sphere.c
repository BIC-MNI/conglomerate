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
    polygons_struct      *polygons, *init_polygons;
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

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    if( get_string_argument( NULL, &initial_filename ) )
    {
        if( input_graphics_file( initial_filename, &format, &n_i_objects,
                                 &i_object_list ) != OK || n_i_objects != 1 ||
            get_object_type(i_object_list[0]) != POLYGONS )
            return( 1 );

        init_polygons = get_polygons_ptr( i_object_list[0] );
        init_points = init_polygons->points;
        ALLOC( init_polygons->points, 1 );
        delete_object_list( n_i_objects, i_object_list );
    }
    else
    {
        init_points = polygons->points;
    }

    flatten_polygons( polygons, init_points, n_iters );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

private  int  create_coefficients(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              **neighbours,
    int              n_fixed,
    int              fixed_indices[],
    Real             *fixed_pos[3],
    int              to_parameters[],
    int              to_fixed_index[],
    int              *n_nodes_involved[],
    int              **node_list[],
    Real             *constants[],
    Real             **node_weights[] )
{
    int              n_equations, node, p, eq, dim, n_nodes_in;
    int              neigh, ind;
    Point            neigh_points[MAX_POINTS_PER_POLYGON];
    Real             x_sphere[MAX_POINTS_PER_POLYGON];
    Real             y_sphere[MAX_POINTS_PER_POLYGON];
    Real             z_sphere[MAX_POINTS_PER_POLYGON];
    Real             weights[MAX_POINTS_PER_POLYGON], radius;
    BOOLEAN          found, ignoring;

    radius = sqrt( get_polygons_surface_area( polygons ) / 4.0 * PI );

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
        if( found && n_neighbours[node] > 3 )
            ++n_equations;
    }

    n_equations *= 3;

    ALLOC( *n_nodes_involved, n_equations );
    ALLOC( *constants, n_equations );
    ALLOC( *node_weights, n_equations );
    ALLOC( *node_list, n_equations );

    eq = 0;
    for_less( node, 0, polygons->n_points )
    {
        n_nodes_in = 0;
        if( to_parameters[node] >= 0 )
            ++n_nodes_in;

        for_less( p, 0, n_neighbours[node] )
        {
            if( to_parameters[neighbours[node][p]] >= 0 )
                ++n_nodes_in;
        }


        if( n_nodes_in == 0 || n_neighbours[node] < 4 )
            continue;

        for_less( p, 0, n_neighbours[node] )
            neigh_points[p] = polygons->points[neighbours[node][p]];

        flatten_around_vertex_to_sphere( radius, &polygons->points[node],
                                         n_neighbours[node], neigh_points,
                                         x_sphere, y_sphere, z_sphere );

        ignoring = FALSE;
        if( !get_prediction_weights_3d( 0.0, 0.0, 0.0,
                                        n_neighbours[node],
                                        x_sphere, y_sphere, z_sphere,
                                        weights ) )
                                         
        {
            print_error( "Error in interpolation weights, ignoring..\n" );
            ignoring = TRUE;
        }

        if( !ignoring )
        {
            for_less( p, 0, n_neighbours[node] )
            {
                if( FABS( weights[p] ) > 100.0 )
                {
                    print_error( "Interpolation weights too high: %d %g\n",
                                 p, weights[p] );
                    ignoring = TRUE;
                }
            }
        }

        if( ignoring )
        {
            for_less( dim, 0, N_DIMENSIONS )
            {
                (*n_nodes_involved)[eq] = 0;
                (*constants)[eq] = 0.0;
                ++eq;
            }
            continue;
        }

#ifdef DEBUG
#define DEBUG
        for_less( dim, 0, N_DIMENSIONS )
        {
            Real  value;
            value = 0.0;
            for_less( p, 0, n_neighbours[node] )
                value += weights[p] * (RPoint_coord(neigh_points[p],dim) - RPoint_coord(polygons->points[node],dim));

            if( !numerically_close( value, 0.0, 1.0e-4 ) )
            {
                print_error( "Error: %g %g\n", value, 0.0 );
                value = 0.0;
                for_less( p, 0, n_neighbours[node] )
                    value += weights[p];
                for_less( p, 0, n_neighbours[node] )
                    print( " %g", weights[p] );
                print( " : %g\n", value );
            }
        }
#endif

        for_less( dim, 0, N_DIMENSIONS )
        {
            (*n_nodes_involved)[eq] = n_nodes_in;
            ALLOC( (*node_list)[eq], (*n_nodes_involved)[eq] );
            ALLOC( (*node_weights)[eq], (*n_nodes_involved)[eq] );

            ind = 0;
            if( to_parameters[node] >= 0 )
            {
                (*node_list)[eq][ind] = 3 * to_parameters[node] + dim;
                (*node_weights)[eq][ind] = 1.0;
                ++ind;
                (*constants)[eq] = 0.0;
            }
            else
                (*constants)[eq] = fixed_pos[dim][to_fixed_index[node]];

            for_less( p, 0, n_neighbours[node] )
            {
                neigh = neighbours[node][p];
                if( to_parameters[neigh] >= 0 )
                {
                    (*node_list)[eq][ind] = 3 * to_parameters[neigh] + dim;
                    (*node_weights)[eq][ind] = -weights[p];
                    ++ind;
                }
                else
                {
                    (*constants)[eq] += -weights[p] *
                                        fixed_pos[dim][to_fixed_index[neigh]];
                }
            }

            ++eq;
        }
    }

#ifdef DEBUG
#define DEBUG
    {
        for_less( eq, 0, n_equations )
        {
            print( "%d: %g : ", eq, (*constants)[eq] );
            for_less( p, 0, (*n_nodes_involved)[eq] )
                print( " %d:%g", (*node_list)[eq][p], (*node_weights)[eq][p] );
            print( "\n" );
        }
    }

#endif

    return( n_equations );
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters )
{
    int              i, point, *n_neighbours, **neighbours;
    int              n_equations, *n_nodes_per_equation, **node_list;
    int              n_fixed, *fixed_indices, size, dim;
    Real             *fixed_pos[3];
    Real             *constants, **node_weights;
    Real             *parameters;
    int              *to_parameters, *to_fixed_index, ind;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

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
        for_less( dim, 0, N_DIMENSIONS )
            fixed_pos[dim][i] = RPoint_coord(
                           polygons->points[fixed_indices[i]], dim );
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
                                       n_neighbours, neighbours,
                                       n_fixed, fixed_indices, fixed_pos,
                                       to_parameters, to_fixed_index,
                                       &n_nodes_per_equation,
                                       &node_list, &constants, &node_weights );

    ALLOC( parameters, 3 * (polygons->n_points - n_fixed) );

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            parameters[3*to_parameters[point]+0] = RPoint_x(init_points[point]);
            parameters[3*to_parameters[point]+1] = RPoint_y(init_points[point]);
            parameters[3*to_parameters[point]+2] = RPoint_z(init_points[point]);
        }
    }

    (void) minimize_lsq( 3 * (polygons->n_points - n_fixed), n_equations,
                         n_nodes_per_equation, node_list, constants,
                         node_weights, n_iters, parameters );

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            fill_Point( polygons->points[point],
                        parameters[3*to_parameters[point]],
                        parameters[3*to_parameters[point]+1],
                        parameters[3*to_parameters[point]+2] );
        }
        else
        {
            fill_Point( polygons->points[point],
                        fixed_pos[0][to_fixed_index[point]],
                        fixed_pos[1][to_fixed_index[point]],
                        fixed_pos[2][to_fixed_index[point]] );
        }
    }

    FREE( parameters );

    FREE( to_parameters );
    FREE( to_fixed_index );
    FREE( fixed_pos[0] );
    FREE( fixed_pos[1] );
    FREE( fixed_pos[2] );
}
