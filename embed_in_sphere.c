#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <priority_queue.h>

private  void  embed_in_sphere(
    polygons_struct  *polygons,
    Real             radius,
    Point            sphere_points[] );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename;
    int                  n_objects;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;
    Point                *sphere_points;

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
    ALLOC( sphere_points, polygons->n_points );

    embed_in_sphere( polygons, 1.0, sphere_points );
    FREE( polygons->points );
    polygons->points = sphere_points;

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

private  int  get_neigh_index(
    int     point_index,
    int     n_neighbours[],
    int     *neighbours[],
    int     neigh )
{
    int   n;

    for_less( n, 0, n_neighbours[point_index] )
    {
        if( neighbours[point_index][n] == neigh )
        {
            return( n );
        }
    }

    handle_internal_error( "get_neigh_index" );

    return( 0 );
}

typedef struct
{
    int             point_index;
    Smallest_int    n_index;
} entry_struct;

public  void  calc_distances_from_point(
    int               n_points,
    Point             points[],
    int               n_neighbours[],
    int               *neighbours[],
    Smallest_int      used_flags[],
    int               start_point,
    int               prev_point,
    int               next_point,
    float             **distances )
{
    int                                     p, n, point_index;
    int                                     neigh, neigh_index, n_index;
    Real                                    dist;
    float                                   new_dist;
    entry_struct                            entry;
    PRIORITY_QUEUE_STRUCT( entry_struct )   queue;

    for_less( p, 0, n_points )
    {
        for_less( n, 0, n_neighbours[p] )
            distances[p][n] = -1.0f;
    }
    for_less( n, 0, n_neighbours[start_point] )
        distances[start_point][n] = 0.0f;

    INITIALIZE_PRIORITY_QUEUE( queue );

    n = 0;

    if( next_point >= 0 )
    {
        while( n < n_neighbours[start_point] &&
               neighbours[start_point][n] != next_point )
            ++n;

        if( n >= n_neighbours[start_point] )
            handle_internal_error(
                      "compute_distances_from_point(): n > n_neighs" );
    }

    n = (n + 1) % n_neighbours[start_point];
    while( neighbours[start_point][n] != prev_point )
    {
        point_index = neighbours[start_point][n];

        if( !used_flags[point_index] )
        {
            dist = distance_between_points( &points[start_point],
                                            &points[point_index] );

            n_index = get_neigh_index( point_index, n_neighbours, neighbours,
                                       start_point );

            if( n_index > 255 )
            {
                handle_internal_error(
                       "Must rewrite code to handle > 255 neighbours" );
            }

            distances[point_index][n_index] = (float) dist;
            entry.point_index = point_index;
            entry.n_index = (Smallest_int) n_index;
            INSERT_IN_PRIORITY_QUEUE( queue, entry, -dist );
        }

        n = (n + 1) % n_neighbours[start_point];

        if( prev_point < 0 && n == 1 )
            break;
    }

    while( !IS_PRIORITY_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_PRIORITY_QUEUE( queue, entry, dist );
        point_index = entry.point_index;
        n_index = (int) entry.n_index;

        if( n_neighbours[point_index] <= 3 )
            continue;

        n = (n_index + 2) % n_neighbours[point_index];
        do
        {
            neigh = neighbours[point_index][n];
            neigh_index = get_neigh_index( neigh, n_neighbours,
                                           neighbours, point_index );
            if( distances[neigh][neigh_index] < 0.0f ||
                distances[neigh][neigh_index] >
                distances[point_index][n_index] )
            {
                new_dist = distances[point_index][n_index] +
                           (float) distance_between_points(
                                                  &points[point_index],
                                                  &points[neigh_index] );

                if( distances[neigh][neigh_index] < 0.0f ||
                    new_dist < distances[neigh][neigh_index] )
                {
                    distances[neigh][neigh_index] = new_dist;
                    entry.point_index = neigh;
                    entry.n_index = (Smallest_int) neigh_index;
                    INSERT_IN_PRIORITY_QUEUE( queue, entry, (Real) -new_dist );
                }
            }

            n = (n + 1) % n_neighbours[point_index];
        }
        while( n != (n_index-1+n_neighbours[point_index]) %
                    n_neighbours[point_index] );
    }

    DELETE_PRIORITY_QUEUE( queue );
}

private  int  create_path_between_points(
    int              n_points,
    int              n_neighbours[],
    int              *neighbours[],
    int              from_point,
    int              to_point,
    int              neigh1,
    int              neigh2,
    float            *distances[],
    int              *path_ptr[] )
{
    int     p, n_path, *path, current, best_neigh, n, neigh, tmp;
    int     n_index1, n_index2, offset, n_to_do;
    float   dist, best_dist;

    n_path = 0;
    path = NULL;
    ADD_ELEMENT_TO_ARRAY( path, n_path, to_point, DEFAULT_CHUNK_SIZE );

    current = to_point;

    do
    {
        if( neigh1 >= 0 && neigh2 >= 0 )
        {
            n_index1 = get_neigh_index( current, n_neighbours, neighbours,
                                        neigh1 );
            n_index2 = get_neigh_index( current, n_neighbours, neighbours,
                                        neigh2 );

            offset = (n_index1 + 1) % n_neighbours[current];
            n_to_do = (n_index2 - n_index1 + n_neighbours[current]) %
                      n_neighbours[current] - 2;
        }
        else
            offset = 0;
            n_to_do = n_neighbours[current];

        best_neigh = 0;
        best_dist = -1.0f;
        for_less( n, 0, n_to_do )
        {
            neigh = (n + offset) % n_neighbours[current];
            dist = distances[current][neigh];
            if( dist >= 0.0f && (best_dist < 0.0f || dist < best_dist) )
            {
                best_dist = dist;
                best_neigh = neighbours[current][neigh];
            }
        }

        current = best_neigh;
        ADD_ELEMENT_TO_ARRAY( path, n_path, current, DEFAULT_CHUNK_SIZE );
        neigh1 = -1;
        neigh2 = -1;
    }
    while( current != from_point );

    for_less( p, 0, n_path/2 )
    {
        tmp = path[p];
        path[p] = path[n_path-1-p];
        path[n_path-1-p] = tmp;
    }

    *path_ptr = path;
    return( n_path );
}

#ifdef NOT_YET

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
#endif

private  int  get_farthest_point_from(
    int    n_points,
    int    n_neighbours[],
    float  *distances[] )
{
    float  farthest_dist, best_dist, dist;
    int    p, furthest,n;

    farthest_dist = -1.0f;
    furthest = 0;
    for_less( p, 0, n_points )
    {
        best_dist = -1.0f;
        for_less( n, 0, n_neighbours[p] )
        {
            dist = distances[p][n];
            if( dist >= 0.0f && (dist < best_dist || best_dist < 0.0f) )
                best_dist = dist;
        }

        if( best_dist >= 0.0f &&
            (farthest_dist < 0.0f || best_dist > farthest_dist) )
        {
            farthest_dist = best_dist;
            furthest = p;
        }
    }

    return( furthest );
}

private  int  get_vertex_on_path(
    int     path_size,
    int     path[],
    Point   points[],
    Real    fractional_distance,
    int     *vertex_index )
{
    int   p;
    Real  total_dist, desired_dist;

    total_dist = 0.0;
    for_less( p, 0, path_size-1 )
    {
        total_dist += distance_between_points( &points[path[p]],
                                               &points[path[p+1]] );
    }

    desired_dist = fractional_distance * total_dist;

    p = 0;
    total_dist = 0.0;
    while( p < path_size-1 && total_dist < desired_dist )
    {
        total_dist += distance_between_points( &points[path[p]],
                                               &points[path[p+1]] );
        ++p;
    }

    *vertex_index = p;

    return( path[p] );
}

private  void  assign_used_flags(
    int           path_size,
    int           path[],
    Smallest_int  used_flags[] )
{
    int  p;

    for_less( p, 0, path_size )
        used_flags[path[p]] = TRUE;
}

private  void  add_to_loop(
    int      *loop_size,
    int      *loop[],
    int      path_size,
    int      path[],
    BOOLEAN  reverse_flag,
    BOOLEAN  closing )
{
    int    start, end, step, p;

    if( reverse_flag )
    {
        start = path_size-1;
        end = -1;
        step = -1;
    }
    else
    {
        start = 0;
        end = path_size;
        step = 1;
    }

    if( *loop_size > 0 )
    {
        if( (*loop)[*loop_size-1] != path[start] )
            handle_internal_error( "add_to_loop() start" );
        start += step;
    }

    if( closing && (*loop)[0] != path[end-step] )
        handle_internal_error( "add_to_loop() end" );

    if( closing )
    {
        end -= step;
        if( path_size == 2 )
            return;
    }

    for( p = start;  p != end;  p += step )
        ADD_ELEMENT_TO_ARRAY( *loop, *loop_size, path[p], DEFAULT_CHUNK_SIZE );
}

private  void  embed_in_sphere(
    polygons_struct  *polygons,
    Real             radius,
    Point            sphere_points[] )
{
    int               point, *n_neighbours, **neighbours, south_pole;
    int               p, n_points, loop_size, *loop;
    float             **distances;
    float             **equator_distances;
    int               left_size, *left_path, right_size, *right_path;
    int               total_neighbours, north_pole;
    int               *long_path, long_path_size, equator_point;
    int               other_equator_point, size1, size2;
    int               *equator1_path, *equator2_path, equator_index;
    Point             *points;
    Smallest_int      *used_flags;

    create_polygon_point_neighbours( polygons, TRUE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    n_points = polygons->n_points;
    points = polygons->points;

    total_neighbours = 0;
    for_less( p, 0, n_points )
        total_neighbours += n_neighbours[p];

    ALLOC( distances, n_points );
    ALLOC( distances[0], total_neighbours );
    for_less( p, 1, n_points )
        distances[p] = &distances[p-1][n_neighbours[p-1]];

    north_pole = 0;

    ALLOC( used_flags, n_points );
    for_less( point, 0, n_points )
        used_flags[point] = FALSE;

    calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                               used_flags, north_pole, -1, -1, distances );

    south_pole = get_farthest_point_from( n_points, n_neighbours, distances );

    print( "NP: %g %g %g\n", RPoint_x(points[north_pole]),
                             RPoint_y(points[north_pole]),
                             RPoint_z(points[north_pole]) );
    print( "SP: %g %g %g\n", RPoint_x(points[south_pole]),
                             RPoint_y(points[south_pole]),
                             RPoint_z(points[south_pole]) );

    long_path_size = create_path_between_points( n_points,
                               n_neighbours, neighbours, north_pole,
                               south_pole, -1, -1, distances, &long_path );

    assign_used_flags( long_path_size, long_path, used_flags );

    equator_point = get_vertex_on_path( long_path_size, long_path, points,
                                        0.5, &equator_index );

    ALLOC( equator_distances, n_points );
    ALLOC( equator_distances[0], total_neighbours );
    for_less( p, 1, n_points )
        equator_distances[p] = &equator_distances[p-1][n_neighbours[p-1]];

    calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                               used_flags, equator_point, -1, -1,
                               equator_distances );

    other_equator_point = get_farthest_point_from( n_points, n_neighbours,
                                                   equator_distances );

    calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                               used_flags, other_equator_point, -1, -1,
                               equator_distances );

    size1 = create_path_between_points( n_points,
                               n_neighbours, neighbours, other_equator_point,
                               north_pole, -1, -1, equator_distances,
                               &equator1_path );

    size2 = create_path_between_points( n_points,
                               n_neighbours, neighbours, other_equator_point,
                               south_pole, -1, -1, equator_distances,
                               &equator2_path );

    while( size1 >= 2 && size2 >= 2 &&
           equator1_path[1] == equator2_path[1] )
    {
        print( "Removing overlap: %d\n", equator1_path[0] );
        --size1;
        --size2;
        for_less( p, 0, size1 )
            equator1_path[p] = equator1_path[p+1];
        for_less( p, 0, size2 )
            equator2_path[p] = equator2_path[p+1];
    }

    other_equator_point = equator1_path[0];

    assign_used_flags( size1, equator1_path, used_flags );
    assign_used_flags( size2, equator2_path, used_flags );

    calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                               used_flags, other_equator_point, -1, -1,
                               equator_distances );

    left_size = create_path_between_points( n_points,
                               n_neighbours, neighbours, other_equator_point,
                               equator_point, long_path[equator_index-1],
                               long_path[equator_index+1], equator_distances,
                               &left_path );

    right_size = create_path_between_points( n_points,
                               n_neighbours, neighbours, other_equator_point,
                               equator_point, long_path[equator_index+1],
                               long_path[equator_index-1], equator_distances,
                               &right_path );

    assign_used_flags( left_size, left_path, used_flags );
    assign_used_flags( right_size, right_path, used_flags );

    loop_size = 0;
    loop = NULL;

    add_to_loop( &loop_size, &loop, right_size, right_path, TRUE, FALSE );
    add_to_loop( &loop_size, &loop, size1, equator1_path, FALSE, FALSE );
    add_to_loop( &loop_size, &loop, equator_index+1, long_path, FALSE, TRUE );

#ifdef NOT_YET
    half_dist = dist_from_north[south_pole] / 2.0;
    dist = 0;
    p = 0;
    while( dist < half_dist )
    {
        dist += distance_between_points( &polygons->points[long_path[p]],
                                         &polygons->points[long_path[p+1]] );
        ++p;
    }

    equator_point = long_path[p];

    (void) compute_distances_from_point( polygons, n_neighbours, neighbours,
                                         &polygons->points[equator_point],
                                         -1, -1.0, FALSE, dist_from_point,
                                         NULL );

    other_equator = -1;
    for_less( point, 0, polygons->n_points )
    {
        if( point == 0 ||
            dist_from_point[point] > dist_from_point[other_equator] )
            other_equator = point;
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
#endif

    FREE( distances[0] );
    FREE( distances );

    FREE( used_flags );
}
