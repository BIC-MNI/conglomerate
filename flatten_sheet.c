#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Real             paramters[],
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename, initial_filename;
    int                  n_objects, n_i_objects, n_iters, i, n_points;
    File_formats         format;
    object_struct        **object_list, **i_object_list;
    polygons_struct      *polygons;
    Real                 *parameters;
    Point                *points;

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

        n_points = get_object_points( i_object_list[0], &points );

        ALLOC( parameters, 2*n_points-4 );

        for_less( i, 0, n_points-2 )
        {
            parameters[2*i] = RPoint_x(points[i+2]);
            parameters[2*i+1] = RPoint_y(points[i+2]);
        }

        delete_object_list( n_i_objects, i_object_list );
    }
    else
    {
        parameters = NULL;
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    flatten_polygons( polygons, parameters, n_iters );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

private  int  create_coefficients(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              **neighbours,
    int              *n_nodes_involved[],
    int              **node_list[],
    Real             *constants[],
    Real             **node_weights[] )
{
    int              n_equations, node, p, eq, dim, n_nodes_in;
    Point            neigh_points[MAX_POINTS_PER_POLYGON];
    Real             flat[2][MAX_POINTS_PER_POLYGON];
    Real             consistency_weights[MAX_POINTS_PER_POLYGON];

    n_equations = 2 * polygons->n_points;

    ALLOC( *n_nodes_involved, n_equations );
    ALLOC( *constants, n_equations );
    ALLOC( *node_weights, n_equations );
    ALLOC( *node_list, n_equations );

    eq = 0;
    for_less( node, 0, polygons->n_points )
    {
        for_less( p, 0, n_neighbours[node] )
            neigh_points[p] = polygons->points[neighbours[node][p]];

        flatten_around_vertex( &polygons->points[node],
                               n_neighbours[node],
                               neigh_points, flat[0], flat[1] );

        if( n_neighbours[node] < 3 ||
            !get_interpolation_weights_2d( 0.0, 0.0, n_neighbours[node],
                                           flat[0], flat[1],
                                           consistency_weights ) )
        {
            print_error( "Error in interpolation weights, using avg..\n" );
            for_less( p, 0, n_neighbours[node] )
                consistency_weights[p] = 1.0 / (Real) n_neighbours[node];
        }

        for_less( dim, 0, 2 )
        {
            n_nodes_in = 0;
            for_less( p, 0, n_neighbours[node] )
            {
                if( neighbours[node][p] > 1 )
                    ++n_nodes_in;
            }

            if( node > 1 )
                ++n_nodes_in;

            (*n_nodes_involved)[eq] = n_nodes_in;
            ALLOC( (*node_list)[eq], (*n_nodes_involved)[eq] );
            ALLOC( (*node_weights)[eq], (*n_nodes_involved)[eq] );

            (*constants)[eq] = 0.0;

            n_nodes_in = 0;
            if( node > 1 )
            {
                (*node_list)[eq][0] = 2 * (node-2) + dim;
                (*node_weights)[eq][0] = 1.0;
                ++n_nodes_in;
            }
            else if( node == 1 && dim == 0 )
                (*constants)[eq] += 1.0;

            for_less( p, 0, n_neighbours[node] )
            {
                if( neighbours[node][p] > 1 )
                {
                    (*node_list)[eq][n_nodes_in] = 2 *(neighbours[node][p]-2)
                                                   + dim;
                    (*node_weights)[eq][n_nodes_in] = -consistency_weights[p];
                    ++n_nodes_in;
                }
                else if( neighbours[node][p] == 1 && dim == 0 )
                    (*constants)[eq] = -consistency_weights[p];
            }

            ++eq;
        }
    }

    return( n_equations );
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Real             parameters[],
    int              n_iters )
{
    int      point, *n_neighbours, **neighbours;
    int      n_equations, *n_nodes_per_equation, **node_list;
    Real     *constants, **node_weights;
    BOOLEAN  alloced;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL );

    n_equations = create_coefficients( polygons,
                                       n_neighbours, neighbours,
                                       &n_nodes_per_equation,
                                       &node_list, &constants, &node_weights );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours, NULL );

    if( parameters == NULL )
    {
        alloced = TRUE;
        ALLOC( parameters, 2 * polygons->n_points - 4 );

        for_less( point, 0, 2 * polygons->n_points - 4 )
            parameters[point] = 0.0;
    }
    else
        alloced = FALSE;

    (void) minimize_lsq( 2 * polygons->n_points - 4, n_equations,
                         n_nodes_per_equation, node_list, constants,
                         node_weights, n_iters, parameters );

    fill_Point( polygons->points[0], 0.0, 0.0, 0.0 );
    fill_Point( polygons->points[1], 1.0, 0.0, 0.0 );

    for_less( point, 0, polygons->n_points - 2 )
    {
        fill_Point( polygons->points[point+2], parameters[2*point],
                    parameters[2*point+1], 0.0 );

        print( "%g\n", parameters[2*point+1] );
    }

    if( alloced )
        FREE( parameters );
}
