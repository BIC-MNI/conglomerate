#include  <internal_volume_io.h>
#include  <bicpl.h>

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

private  void  create_neighbours(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              *neighbours[] )
{
    int   v0, v1, point, point1, point2, size, indices[MAX_POINTS_PER_POLYGON];
    int   poly, vertex;

    for_less( point, 0, polygons->n_points )
        n_neighbours[point] = 0;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( point, 0, size )
            indices[point] = polygons->indices[
                              POINT_INDEX(polygons->end_indices,poly,point)];

        for_less( v0, 0, size )
        {
            point1 = indices[v0];
            n_neighbours[point1] += size - 2;
        }
    }

    for_less( point, 0, polygons->n_points )
    {
        ALLOC( neighbours[point], n_neighbours[point] );
        n_neighbours[point] = 0;
    }

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( vertex, 0, size )
            indices[vertex] = polygons->indices[
                              POINT_INDEX(polygons->end_indices,poly,vertex)];

        for_less( v0, 0, size )
        {
            point1 = indices[v0];

            for_less( v1, 1, size-1 )
            {
                point2 = indices[(v0 + v1) % size];
                neighbours[point1][n_neighbours[point1]] = point2;
                ++n_neighbours[point1];
            }
        }
    }
}

private  void  create_distances(
    int              n_points,
    Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    int              north_pole,
    float            distances[] )
{
    int              point, current, n_changed, iter, neigh, n;
    Smallest_int     *changed[2];
    float            new_dist;

    ALLOC( changed[0], n_points );
    ALLOC( changed[1], n_points );

    for_less( point, 0, n_points )
    {
        distances[point] = -1.0f;
        changed[0][point] = FALSE;
    }
    distances[north_pole] = 0.0f;
    changed[0][north_pole] = TRUE;
    current = 0;

    iter = 0;
    n_changed = 1;
    while( n_changed > 0 )
    {
        n_changed = 0;

        for_less( point, 0, n_points )
            changed[1-current][point] = FALSE;

        for_less( point, 0, n_points )
        {
            if( !changed[current][point] )
                continue;

            for_less( neigh, 0, n_neighbours[point] )
            {
                n = neighbours[point][neigh];
                new_dist = distances[point] +
                           (float) distance_between_points( &points[point],
                                                            &points[n] );
                if( distances[n] < 0.0f || new_dist < distances[n] )
                {
                    ++n_changed;
                    distances[n] = new_dist;
                    changed[1-current][n] = TRUE;
                }
            }
        }

        current = 1 - current;
        ++iter;
        print( "Iter %3d: %d\n", iter, n_changed );
    }

    FREE( changed[0] );
    FREE( changed[1] );
}

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
    int     current_ind;
    float   height, ratio, sum_dist, to_point_dist;
    Point   start_point, prev_point, next_point;

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

    do
    {
        ind = current_ind;
        while( vertical[polygons->indices[POINT_INDEX(polygons->end_indices,
                                       current_poly,(ind+1)%size)]] <= height )
        {
            ind = (ind + 1) % size;
        }

        p0 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                              current_poly,ind)];
        p1 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                              current_poly,(ind+1)%size)];

        ratio = (height - vertical[p0]) / (vertical[p1] - vertical[p0]);
        INTERPOLATE_POINTS( next_point, polygons->points[p0],
                            polygons->points[p1], ratio );
        sum_dist += (float) distance_between_points( &prev_point, &next_point);
        if( p0 == point )
            to_point_dist = sum_dist;

        prev_point = next_point;

        current_poly = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                            current_poly,ind)];
        size = GET_OBJECT_SIZE( *polygons, current_poly );
        current_ind = 0;
        while( polygons->indices[POINT_INDEX(polygons->end_indices,
                                             current_poly,current_ind)] != p0 )
            ++current_ind;
    }
    while( current_poly != poly );

    sum_dist += (float) distance_between_points( &prev_point, &start_point );

    return( to_point_dist / sum_dist );
}

private  void  create_2d_coordinates(
    polygons_struct  *polygons,
    int              north_pole )
{
    int               point, *n_neighbours, **neighbours, south_pole, path_size;
    int               *path, *polygons_in_path, p;
    float             *vertical, *horizontal;
    progress_struct   progress;
    Real              x, y, z;

    ALLOC( vertical, polygons->n_points );
    ALLOC( horizontal, polygons->n_points );
    ALLOC( n_neighbours, polygons->n_points );
    ALLOC( neighbours, polygons->n_points );

    check_polygons_neighbours_computed( polygons );

    create_neighbours( polygons, n_neighbours, neighbours );

    create_distances( polygons->n_points, polygons->points,
                      n_neighbours, neighbours, north_pole, vertical );

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

    create_distances( polygons->n_points, polygons->points,
                      n_neighbours, neighbours, south_pole, horizontal );

    for_less( point, 0, polygons->n_points )
    {
        vertical[point] = 1.0f - vertical[point] /
                          (vertical[point] + horizontal[point] );
    }

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Computing Horizontal Coord" );

    for_less( point, 0, polygons->n_points )
    {
        horizontal[point] = get_horizontal_coord( point, polygons, n_neighbours,
                   neighbours, path_size, path, polygons_in_path, vertical );
        update_progress_report( &progress, point+1 );
    }

    terminate_progress_report( &progress );

    for_less( point, 0, polygons->n_points )
        FREE( neighbours[point] );
    FREE( n_neighbours );
    FREE( neighbours );

/*
    write_values_to_file( "vertical.txt", polygons->n_points, vertical );
    write_values_to_file( "horizontal.txt", polygons->n_points, horizontal );
*/

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
