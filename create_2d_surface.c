#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>
#include  <special_geometry.h>

#define  DEBUG

private  void  create_2d_coordinates(
    polygons_struct  *polygons,
    int              north_pole );

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

    create_2d_coordinates( polygons, 0 );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

#ifdef DEBUG
private  void  write_values_to_file(
    STRING  filename,
    int     n_points,
    float   values[] )
{
    int   point;
    FILE  *file;

    (void) open_file( filename, WRITE_FILE, ASCII_FORMAT, &file );
    for_less( point, 0, n_points )
    {
        (void) output_float( file, values[point] );
        (void) output_newline( file );
    }
    (void) close_file( file );
}
#endif

private  int  create_path_between_poles(
    int              n_points,
    Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    int              north_pole,
    int              south_pole,
    float            vertical[],
    int              *path_ptr[] )
{
    int     n_path, *path, current, best_neigh, n, neigh;
    float   dist, best_dist;

    n_path = 0;
    path = NULL;
    ADD_ELEMENT_TO_ARRAY( path, n_path, south_pole, DEFAULT_CHUNK_SIZE );

    current = south_pole;

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
    while( current != north_pole );

    *path_ptr = path;
    return( n_path );
}

private  int  get_polygon_containing_vertices(
    polygons_struct  *polygons,
    int              p0,
    int              p1 )
{
    int     p, poly, size, v0, v1;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( v0, 0, size )
        {
            p = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                  poly,v0)];
            if( p == p0 )
                break;
        }
        if( v0 == size )
            continue;

        for_less( v1, 0, size )
        {
            p = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                  poly,v1)];
            if( p == p1 )
                break;
        }
        if( v1 < size )
            break;
    }

    if( poly >= polygons->n_items )
        handle_internal_error( "poly >= polygons->n_items" );

    return( poly );
}





private  float  get_horizontal_coord(
    int              point,
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              *neighbours[],
    int              n_path,
    int              path[],
    int              polygons_in_path[],
    float            vertical[] )
{
    int     ind, path_index, p, p0, p1, poly, current_poly, size, v0, v1;
    int     current_ind, start_index;
    float   height, ratio, sum_dist, to_point_dist, dx, dy, dz;
    Point   start_point, prev_point, next_point, *point0, *point1;

    height = vertical[point];
    if( height <= 0.0f || height >= 1.0f )
        return( 0.0f );

    path_index = 0;
    while( vertical[path[path_index+1]] <= height )
        ++path_index;

    p0 = path[path_index];
    p1 = path[path_index+1];
    poly = polygons_in_path[path_index];

    size = GET_OBJECT_SIZE( *polygons, poly );
    for_less( v0, 0, size )
    {
        p = polygons->indices[POINT_INDEX(polygons->end_indices,
                                              poly,v0)];
        if( p == p0 )
            break;
    }
    if( v0 == size )
        handle_internal_error( "v0 == size" );

    for_less( v1, 0, size )
    {
        p = polygons->indices[POINT_INDEX(polygons->end_indices,
                                              poly,v1)];
        if( p == p1 )
            break;
    }
    if( v1 == size )
        handle_internal_error( "v1 == size" );

    ratio = (height - vertical[p0]) / (vertical[p1] - vertical[p0]);
    INTERPOLATE_POINTS( start_point, polygons->points[p0], polygons->points[p1],
                        ratio );

    prev_point = start_point;
    sum_dist = 0.0f;
    current_poly = poly;
    current_ind = v0;
    start_index = START_INDEX(polygons->end_indices,current_poly);
    to_point_dist = -1.0f;

    do
    {
        ind = current_ind;
        p0 = polygons->indices[start_index+ind];
        if( p0 == point )
        {
            to_point_dist = sum_dist + (float)
                            distance_between_points( &prev_point,
                                                     &polygons->points[point] );
        }
        ind = (ind + 1) % size;
        p1 = polygons->indices[start_index+ind];
        while( vertical[p1] <= height )
        {
            ind = (ind + 1) % size;
            p0 = p1;
            p1 = polygons->indices[start_index+ind];

            if( p0 == point )
            {
                to_point_dist = sum_dist + (float)
                          distance_between_points( &prev_point,
                                                   &polygons->points[point] );
            }
        }

        ind = (ind - 1 + size) % size;

        ratio = (height - vertical[p0]) / (vertical[p1] - vertical[p0]);

        point0 = &polygons->points[p0];
        point1 = &polygons->points[p1];

        fill_Point( next_point,
                    Point_x(*point0)+ratio*(Point_x(*point1)-Point_x(*point0)),
                    Point_y(*point0)+ratio*(Point_y(*point1)-Point_y(*point0)),
                    Point_z(*point0)+ratio*(Point_z(*point1)-Point_z(*point0)));

        dx = Point_x(next_point) - Point_x(prev_point);
        dy = Point_y(next_point) - Point_y(prev_point);
        dz = Point_z(next_point) - Point_z(prev_point);
        sum_dist += sqrtf( dx * dx + dy * dy + dz * dz );

        prev_point = next_point;

        current_poly = polygons->neighbours[start_index+ind];
        size = GET_OBJECT_SIZE( *polygons, current_poly );
        current_ind = 0;
        start_index = START_INDEX(polygons->end_indices,current_poly);
        while( polygons->indices[start_index + current_ind] != p0 )
            ++current_ind;
    }
    while( current_poly != poly );

    sum_dist += (float) distance_between_points( &prev_point, &start_point );

    if( to_point_dist < 0.0f )
    {
        to_point_dist = -1.0f;
    }
    else
        to_point_dist /= sum_dist;

    return( to_point_dist );
}

private  void  create_2d_coordinates(
    polygons_struct  *polygons,
    int              north_pole )
{
    int               point, *n_neighbours, **neighbours, south_pole, path_size;
    int               *path, *polygons_in_path, p, n_not_done;
    float             *vertical, *horizontal;
    float             *distances, neigh_dist, best_dist, best_hor;
    int               n, neigh, current;
    progress_struct   progress;
    Real              x, y, z, dummy;
    PRIORITY_QUEUE_STRUCT( int )   queue;

    ALLOC( vertical, polygons->n_points );
    ALLOC( horizontal, polygons->n_points );

    check_polygons_neighbours_computed( polygons );

    create_polygon_point_neighbours( polygons, TRUE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    (void) compute_distances_from_point( polygons, n_neighbours, neighbours,
                                         &polygons->points[north_pole],
                                         -1, -1.0, FALSE, vertical, NULL );

    south_pole = 0;
    for_less( point, 0, polygons->n_points )
    {
        if( point == 0 || vertical[point] > vertical[south_pole] )
            south_pole = point;
    }

    path_size = create_path_between_poles( polygons->n_points, polygons->points,
                               n_neighbours, neighbours, north_pole,
                               south_pole, vertical, &path );

    ALLOC( polygons_in_path, path_size-1 );
    for_less( p, 0, path_size-1 )
    {
        polygons_in_path[p] = get_polygon_containing_vertices( polygons,
                                                      path[p], path[p+1] );
    }

    (void) compute_distances_from_point( polygons, n_neighbours, neighbours,
                                         &polygons->points[south_pole],
                                         -1, -1.0, FALSE, horizontal, NULL );

    for_less( point, 0, polygons->n_points )
    {
        vertical[point] = horizontal[point] /
                          (vertical[point] + horizontal[point] );
    }

#ifdef DEBUG
    write_values_to_file( "vertical.txt", polygons->n_points, vertical );
#endif

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Computing Horizontal Coord" );

    n_not_done = 0;
    for_less( point, 0, polygons->n_points )
    {
        horizontal[point] = get_horizontal_coord( point, polygons, n_neighbours,
                   neighbours, path_size, path, polygons_in_path, vertical );
        update_progress_report( &progress, point+1 );

        if( horizontal[point] < 0.0f )
            ++n_not_done;
    }

    terminate_progress_report( &progress );

    if( n_not_done > 0 )
    {
        print( "Found %d that could not be assigned horizontal coord.\n",
               n_not_done );
        ALLOC( distances, polygons->n_points );

        initialize_progress_report( &progress, FALSE, polygons->n_points,
                                    "Correcting Horizontal Coord" );

        for_less( point, 0, polygons->n_points )
        {
            if( horizontal[point] < 0.0f )
            {
                for_less( p, 0, polygons->n_points )
                    distances[p] = -1.0f;

                INITIALIZE_PRIORITY_QUEUE( queue );
                INSERT_IN_PRIORITY_QUEUE( queue, point, 0.0 );
                best_dist = -1.0f;
                distances[point] = 0.0f;

                while( !IS_PRIORITY_QUEUE_EMPTY(queue) )
                {
                    REMOVE_FROM_PRIORITY_QUEUE( queue, current, dummy );
                    if( best_dist >= 0.0f &&
                        distances[current] > (float) best_dist )
                        break;

                    for_less( n, 0, n_neighbours[current] )
                    {
                        neigh = neighbours[current][n];
                        neigh_dist = distances[neigh];
                        if( neigh_dist >= 0.0f &&
                            distances[current] >= neigh_dist ) 
                            continue;

                        neigh_dist = (float) ((Real) distances[current] + 
                                              distance_between_points(
                                        &polygons->points[current],
                                        &polygons->points[neigh] ));

                        if( distances[neigh] < 0.0f ||
                            neigh_dist < distances[neigh] )
                        {
                            INSERT_IN_PRIORITY_QUEUE( queue, neigh,
                                                      (Real) -neigh_dist );
                            distances[neigh] = neigh_dist;
                            if( horizontal[neigh] >= 0.0f &&
                                (best_dist < 0.0f || neigh_dist < best_dist) )
                            {
                                best_dist = neigh_dist;
                                best_hor = horizontal[neigh];
                            }
                        }
                    }
                }

                if( best_dist < 0.0f )
                    handle_internal_error( "Dang" );

                horizontal[point] = best_hor;

                DELETE_PRIORITY_QUEUE( queue );
            }

            update_progress_report( &progress, point+1 );
        }

        FREE( distances );
        terminate_progress_report( &progress );
    }

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     NULL, NULL );

#ifdef DEBUG
/*
    write_values_to_file( "vertical.txt", polygons->n_points, vertical );
    write_values_to_file( "horizontal.txt", polygons->n_points, horizontal );
*/
#endif

    for_less( point, 0, polygons->n_points )
    {
        map_uv_to_sphere( (Real) horizontal[point], (Real) vertical[point],
                          &x, &y, &z );
        fill_Point( polygons->points[point], x, y, z );
    }

    FREE( polygons_in_path );
    FREE( path );
    FREE( vertical );
    FREE( horizontal );
}
