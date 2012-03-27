#include  <volume_io.h>
#include  <bicpl.h>
#include  <special_geometry.h>

private  void  create_2d_coordinates(
    polygons_struct  *polygons );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename;
    int                  n_objects;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    create_2d_coordinates( polygons );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

private  int  create_path_between_nodes(
    int              n_points,
    Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    int              start_point,
    int              end_point,
    float            vertical[],
    int              *path_ptr[] )
{
    int     n_path, *path, current, best_neigh, n, neigh;
    float   dist, best_dist;

    n_path = 0;
    path = NULL;
    ADD_ELEMENT_TO_ARRAY( path, n_path, end_point, DEFAULT_CHUNK_SIZE );

    current = end_point;

    do
    {
        best_dist = 0.0f;
        for_less( neigh, 0, n_neighbours[current] )
        {
            n = neighbours[current][neigh];
            dist = (float) distance_between_points( &points[current],
                                                    &points[n] ) +
                   vertical[n];

            if( neigh == 0 || dist < best_dist )
            {
                best_dist = dist;
                best_neigh = n;
            }
        }

        current = best_neigh;
        ADD_ELEMENT_TO_ARRAY( path, n_path, current, DEFAULT_CHUNK_SIZE );
    }
    while( current != start_point );

    *path_ptr = path;
    return( n_path );
}

private  void  compute_point_position(
    Real   x1,
    Real   dist02,
    Real   dist12,
    Point  *point )
{
    Real  x, y, rem;

    x = (x1 * x1 + dist02 * dist02 - dist12 * dist12) / (2.0 * x1);
    rem = dist02 * dist02 - x * x;
    if( rem <= 0.0 )
        y = 0.0;
    else
        y = sqrt( dist02 * dist02 - x * x );

    fill_Point( *point, x, y, 0.0 );
}

private  void  create_2d_coordinates(
    polygons_struct  *polygons )
{
    int                  point, *n_neighbours, **neighbours, origin, opposite;
    int                  *path, *vertices_in_path, n, path_index, p0, p1, p2;
    int                  current_point, n_points, start_point, vertex;
    int                  path_size;
    Real                 axis_length, dist1, dist2;
    float                *distances_from_origin, *distances_from_opposite;
    Smallest_int         *interior_flags;
    progress_struct      progress;
    QUEUE_STRUCT( int )  queue;

    check_polygons_neighbours_computed( polygons );

    create_polygon_point_neighbours( polygons, TRUE, &n_neighbours,
                                     &neighbours, &interior_flags, NULL );
    n_points = polygons->n_points;

    for_less( origin, 0, n_points )
    {
        if( !interior_flags[origin] )
            break;
    }

    if( origin >= n_points )
    {
        print_error( "Surface is not an open sheet.\n" );
        return;
    }

    print( "Getting distances from origin.\n" );

    ALLOC( distances_from_origin, n_points );
    (void) compute_distances_from_point( polygons, n_neighbours, neighbours,
                                         &polygons->points[origin],
                                         -1, -1.0, FALSE,
                                         distances_from_origin, NULL );

    opposite = -1;
    for_less( point, 0, n_points )
    {
        if( !interior_flags[point] &&
            (opposite < 0 ||
             distances_from_origin[point] > distances_from_origin[opposite]) )
        {
            opposite = point;
        }
    }

    print( "Getting distances from opposite.\n" );

    ALLOC( distances_from_opposite, n_points );
    (void) compute_distances_from_point( polygons, n_neighbours, neighbours,
                                         &polygons->points[opposite],
                                         -1, -1.0, FALSE,
                                         distances_from_opposite, NULL );

    path_size = create_path_between_nodes( polygons->n_points, polygons->points,
                               n_neighbours, neighbours, origin,
                               opposite, distances_from_origin, &path );

    ALLOC( vertices_in_path, n_points );
    for_less( point, 0, n_points )
        vertices_in_path[point] = FALSE;

    for_less( path_index, 0, path_size )
        vertices_in_path[path[path_index]] = TRUE;

    for_less( path_index, 1, path_size-1 )
    {
        p0 = path[path_index-1];
        p1 = path[path_index];
        p2 = path[path_index+1];

        for_less( vertex, 0, n_neighbours[p1] )
        {
            if( neighbours[p1][vertex] == p0 )
                break;
        }

        vertex = (vertex + 1) % n_neighbours[p1];
        start_point = neighbours[p1][vertex];
        if( start_point != p2 && !vertices_in_path[start_point] )
            break;
    }

    if( path_index >= path_size-1 )
    {
        print_error( "Cannot find path\n" );
        return;
    }

    INITIALIZE_QUEUE( queue );
    vertices_in_path[start_point] = TRUE;
    INSERT_IN_QUEUE( queue, start_point );

    while( !IS_QUEUE_EMPTY(queue) )
    {
        REMOVE_FROM_QUEUE( queue, current_point );

        for_less( n, 0, n_neighbours[current_point] )
        {
            if( !vertices_in_path[neighbours[current_point][n]] )
            {
                vertices_in_path[neighbours[current_point][n]] = TRUE;
                INSERT_IN_QUEUE( queue, neighbours[current_point][n] );
            }
        }
    }

    DELETE_QUEUE( queue );

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Computing Coords" );

    axis_length = (Real) distances_from_origin[opposite];

    for_less( point, 0, polygons->n_points )
    {
        dist1 = (Real) distances_from_origin[point];
        dist2 = (Real) distances_from_opposite[point];

        compute_point_position( axis_length, dist1, dist2,
                                &polygons->points[point] );

        if( !vertices_in_path[point] )
            Point_y(polygons->points[point]) =-Point_y(polygons->points[point]);

        update_progress_report( &progress, point+1 );
    }

    terminate_progress_report( &progress );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     NULL, NULL );

    FREE( vertices_in_path );
    FREE( path );
    FREE( distances_from_origin );
    FREE( distances_from_opposite );
}
