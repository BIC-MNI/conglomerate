#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>
#include  <priority_queue.h>

private  int  convert_manifold_to_sheet(
    polygons_struct  *polygons,
    int              north_pole,
    int              south_pole,
    Point            *new_points[],
    Point            *new_flat_points[],
    int              *new_indices[],
    int              *n_fixed,
    int              *fixed_list[] );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename, dest_init_filename;
    STRING               dest_fixed_filename;
    int                  p, n_objects, north_pole, south_pole, new_n_points;
    FILE                 *file;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;
    Point                *init_points, *new_points;
    int                  *new_indices, n_fixed, *fixed_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_string_argument( NULL, &dest_init_filename ) ||
        !get_string_argument( NULL, &dest_fixed_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj flatten_init.obj\n",
                      argv[0] );
        print_error( "          fixed.txt [north_pole]  [south_pole]\n" );
        return( 1 );
    }

    (void) get_int_argument( 0, &north_pole );
    (void) get_int_argument( -1, &south_pole );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    for_less( p, 0, polygons->n_items )
        if( GET_OBJECT_SIZE(*polygons,p) != 3 )
            break;

    if( p < polygons->n_items )
    {
        print_error( "Surface must be triangles only.\n" );
        return( 1 );
    }

    new_n_points = convert_manifold_to_sheet( polygons, north_pole,
                                              south_pole, &new_points,
                                              &init_points, &new_indices,
                                              &n_fixed, &fixed_list );

    FREE( polygons->points );
    polygons->points = new_points;
    polygons->n_points = new_n_points;
    polygons->indices = new_indices;
    (void) output_graphics_file( dest_filename, format, n_objects, object_list);

    FREE( polygons->points );
    polygons->points = init_points;
    (void) output_graphics_file( dest_init_filename, format, n_objects,
                                 object_list);

    delete_object_list( n_objects, object_list );

    if( open_file( dest_fixed_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK)
        return( 1 );

    for_less( p, 0, n_fixed )
    {
        if( output_int( file, fixed_list[p] ) != OK ||
            output_newline( file ) != OK )
        {
            print_error( "Error writing file\n" );
        }
    }

    (void) close_file( file );

    output_alloc_to_file( NULL );

    return( 0 );
}

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
    int               start_point,
    float             **distances )
{
    int                                     p, n, point_index;
    int                                     neigh, neigh_index, n_index;
    int                                     offset, n_to_do;
    Real                                    dist;
    float                                   new_dist, neigh_dist, current_dist;
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

    for_less( n, 0, n_neighbours[start_point] )
    {
        point_index = neighbours[start_point][n];

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

    while( !IS_PRIORITY_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_PRIORITY_QUEUE( queue, entry, dist );
        point_index = entry.point_index;
        n_index = (int) entry.n_index;

        if( n_neighbours[point_index] <= 3 )
            continue;

        current_dist = distances[point_index][n_index];

        offset = (n_index + 2) % n_neighbours[point_index];
        n_to_do = n_neighbours[point_index] - 3;

        for_less( n, 0, n_to_do )
        {
            n_index = (n + offset) % n_neighbours[point_index];

            neigh = neighbours[point_index][n_index];
            neigh_index = get_neigh_index( neigh, n_neighbours,
                                           neighbours, point_index );

            neigh_dist = distances[neigh][neigh_index];
            if( neigh_dist < 0.0f || neigh_dist > current_dist )
            {
                new_dist = current_dist +
                           (float) distance_between_points(
                                      &points[point_index], &points[neigh] );

                if( neigh_dist < 0.0f || new_dist < neigh_dist )
                {
                    distances[neigh][neigh_index] = new_dist;

                    entry.point_index = neigh;
                    entry.n_index = (Smallest_int) neigh_index;
                    INSERT_IN_PRIORITY_QUEUE( queue, entry, (Real) -new_dist );
                }
            }
        }
    }

    DELETE_PRIORITY_QUEUE( queue );
}

private  int  create_path_between_points(
    int              n_points,
    Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    int              from_point,
    int              to_point,
    float            *distances[],
    int              *path_ptr[] )
{
    int     p, n_path, *path, current, best_neigh, n, neigh, tmp;
    int     n_to_do, neigh_index, n_index;
    float   dist, best_dist;

    n_path = 0;
    path = NULL;
    ADD_ELEMENT_TO_ARRAY( path, n_path, to_point, DEFAULT_CHUNK_SIZE );

    current = to_point;
    neigh_index = 0;
    n_to_do = n_neighbours[current];

    do
    {
        best_neigh = 0;
        best_dist = -1.0f;
        for_less( n, 0, n_to_do )
        {
            n_index = (neigh_index + 2 + n) % n_neighbours[current];
            neigh = neighbours[current][n_index];
            if( distances[current][n_index] < 0.0f )
                continue;

            dist = distances[current][n_index] -
                      (float) distance_between_points( &points[current],
                                                       &points[neigh] );
            if( dist >= 0.0f && (best_dist < 0.0f || dist < best_dist) )
            {
                best_dist = dist;
                best_neigh = neigh;
            }
        }

        if( best_dist < 0.0f )
        {
            if( n_path > 1 )
                handle_internal_error( "n_path > 1 " );
            break;
        }

        neigh_index = get_neigh_index( best_neigh, n_neighbours, neighbours,
                                       current );

        current = best_neigh;
        n_to_do = n_neighbours[current] - 3;
        ADD_ELEMENT_TO_ARRAY( path, n_path, current, DEFAULT_CHUNK_SIZE );
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

private  int  get_farthest_point_from(
    int           n_points,
    int           n_neighbours[],
    float         *distances[] )
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

    if( farthest_dist < 0.0f )
        handle_internal_error( "get_farthest_point_from" );

    return( furthest );
}

private  BOOLEAN   is_to_left_of_meridian(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              *neighbours[],
    int              poly,
    int              path_size,
    int              path[],
    int              new_ids[] )
{
    int     size, vertex, p1, p2, path_index, n_to_neigh, n_to_next;
    int     prev_index, next_index, neigh_index;

    size = GET_OBJECT_SIZE( *polygons, poly );
    for_less( vertex, 0, size )
    {
        p1 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                           vertex)];

        if( new_ids[p1] >= 0 )
            break;
    }

    do
    {
        vertex = (vertex + 1) % size;
        p2 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                           vertex)];
    }
    while( p2 != p1 && new_ids[p2] >= 0 ||
           p2 == path[0] || p2 == path[path_size-1] );

    if( p2 == p1 )
        handle_internal_error( "is_to_left_of_meridian" );

    path_index = 1;
    while( path_index < path_size-1 && path[path_index] != p1 )
        ++path_index;

    if( path_index >= path_size-1 )
        handle_internal_error( "path_index >= path_size-1" );

    prev_index = get_neigh_index( p1, n_neighbours, neighbours,
                                  path[path_index-1] );
    next_index = get_neigh_index( p1, n_neighbours, neighbours,
                                  path[path_index+1] );
    neigh_index = get_neigh_index( p1, n_neighbours, neighbours, p2 );

    n_to_neigh = (neigh_index - prev_index + n_neighbours[p1]) %
                 n_neighbours[p1];
    n_to_next = (next_index - prev_index + n_neighbours[p1]) % n_neighbours[p1];

    if( n_to_neigh <= 0 || n_to_neigh >= n_neighbours[p1] ||
        n_to_next <= 0 || n_to_next >= n_neighbours[p1] )
        handle_internal_error( "neighbours" );
       
    return( n_to_neigh < n_to_next );
}

private  int  convert_manifold_to_sheet(
    polygons_struct  *polygons,
    int              north_pole,
    int              south_pole,
    Point            *new_points[],
    Point            *new_flat_points[],
    int              *new_indices[],
    int              *n_fixed,
    int              *fixed_list[] )
{
    int               *n_neighbours, **neighbours, ind;
    int               p, n_points, poly, new_n_points, *new_ids, neigh_index;
    float             **distances;
    Real              x, y, total_length, current_length, north_length;
    Real              south_length, width, scale, z;
    int               total_neighbours;
    int               *path, path_size;
    int               size, vertex, p1;
    Point             *points;
    BOOLEAN           use_new_ids;
    progress_struct   progress;

    n_points = polygons->n_points;
    points = polygons->points;

    create_polygon_point_neighbours( polygons, TRUE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    total_neighbours = 0;
    for_less( p, 0, n_points )
        total_neighbours += n_neighbours[p];

    ALLOC( distances, n_points );
    ALLOC( distances[0], total_neighbours );
    for_less( p, 1, n_points )
        distances[p] = &distances[p-1][n_neighbours[p-1]];

    calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                               north_pole, distances );

    if( south_pole < 0 || south_pole >= n_points )
    {
        south_pole = get_farthest_point_from( n_points, n_neighbours,
                                              distances );
    }

    print( "NP %d: %g %g %g\n", north_pole, RPoint_x(points[north_pole]),
                                            RPoint_y(points[north_pole]),
                                            RPoint_z(points[north_pole]) );
    print( "SP %d: %g %g %g\n", south_pole, RPoint_x(points[south_pole]),
                                            RPoint_y(points[south_pole]),
                                            RPoint_z(points[south_pole]) );

    path_size = create_path_between_points( n_points, points,
                               n_neighbours, neighbours, north_pole,
                               south_pole, distances, &path );

    if( path_size == 1 )
        handle_internal_error( "path_size" );

    {
        int  p1, p2, p3, n1, n3;

        for_less( p, 1, path_size-1 )
        {
            p1 = path[p-1];
            p2 = path[p];
            p3 = path[p+1];

            n1 = get_neigh_index( p2, n_neighbours, neighbours, p1 );
            n3 = get_neigh_index( p2, n_neighbours, neighbours, p3 );

            if( n1 == (n3 + 1) % n_neighbours[p2] ||
                n3 == (n1 + 1) % n_neighbours[p2] )
                handle_internal_error( "path" );
        }
    }

    FREE( distances[0] );
    FREE( distances );

    new_n_points = n_points + path_size - 2;
    ALLOC( *new_indices, NUMBER_INDICES(*polygons) );
    ALLOC( *new_points, new_n_points );
    ALLOC( *new_flat_points, new_n_points );

    ALLOC( new_ids, n_points );
    for_less( p, 0, n_points )
        new_ids[p] = -1;

    for_less( p, 0, n_points )
        (*new_points)[p] = points[p];

    for_less( p, 1, path_size-1 )
    {
        new_ids[path[p]] = n_points + p - 1;
        (*new_points)[n_points+p-1] = points[path[p]];
    }

    *n_fixed = 2 + 2 * (path_size-2) + n_neighbours[north_pole] - 1 +
                                       n_neighbours[south_pole] - 1;
    ALLOC( *fixed_list, *n_fixed );
    (*fixed_list)[0] = north_pole;
    (*fixed_list)[1] = south_pole;
    ind = 2;
    for_less( p, 1, path_size-1 )
    {
        (*fixed_list)[ind] = path[p];
        ++ind;
        (*fixed_list)[ind] = new_ids[path[p]];
        ++ind;
    }

    neigh_index = get_neigh_index( north_pole, n_neighbours, neighbours,
                                   path[1] );
    for_less( p, 1, n_neighbours[north_pole] )
    {
        (*fixed_list)[ind] = neighbours[north_pole][(neigh_index+p)%
                                               n_neighbours[north_pole]];
        ++ind;
    }

    neigh_index = get_neigh_index( south_pole, n_neighbours, neighbours,
                                   path[path_size-2] );
    for_less( p, 1, n_neighbours[south_pole] )
    {
        (*fixed_list)[ind] = neighbours[south_pole][(neigh_index+p)%
                                               n_neighbours[south_pole]];
        ++ind;
    }

    if( ind != *n_fixed )
        handle_internal_error( "n_fixed" );

    if( getenv( "NO_POLES" ) != NULL )
        *n_fixed -= n_neighbours[north_pole] - 1 + n_neighbours[south_pole] - 1;

    for_less( p, 0, new_n_points )
        fill_Point( (*new_flat_points)[p], 0.5, 0.5, 0.0 );
    fill_Point( (*new_flat_points)[south_pole], 0.5, 0.0, 0.0 );
    fill_Point( (*new_flat_points)[north_pole], 0.5, 1.0, 0.0 );

    total_length = 0.0;
    for_less( p, 0, path_size-1 )
        total_length += distance_between_points( &polygons->points[path[p]],
                                                 &polygons->points[path[p+1]] );

    if( getenv("SCALE") == NULL ||
        sscanf( getenv("SCALE"), "%lf", &scale ) != 1 )
        scale = 1.0;

    current_length = 0.0;
    for_less( p, 1, path_size-1 )
    {
        current_length += distance_between_points( &polygons->points[path[p-1]],
                                                   &polygons->points[path[p]] );
        y = 1.0 - current_length / total_length;
        z = cos( y * PI );
        width = scale * sqrt( 1.0 - z * z );
        x = 0.5 - width / 2.0;

        fill_Point( (*new_flat_points)[path[p]], x, y, 0.0 );
        fill_Point( (*new_flat_points)[new_ids[path[p]]], 1.0 - x, y, 0.0 );
    }

    /*--- around north pole */

    neigh_index = get_neigh_index( north_pole, n_neighbours, neighbours,
                                   path[1] );
    north_length = 0.0;
    for_less( p, 0, n_neighbours[north_pole] )
    {
        north_length += distance_between_points(
                        &polygons->points[neighbours[north_pole]
                                [(neigh_index+p)%n_neighbours[north_pole]]],
                        &polygons->points[neighbours[north_pole]
                                [(neigh_index+p+1)%n_neighbours[north_pole]]] );
    }

    current_length = 0.0;
    for_less( p, 1, n_neighbours[north_pole] )
    {
        current_length += distance_between_points(
                        &polygons->points[neighbours[north_pole]
                                [(neigh_index+p-1+n_neighbours[north_pole])%
                                  n_neighbours[north_pole]]],
                        &polygons->points[neighbours[north_pole]
                                [(neigh_index+p)%n_neighbours[north_pole]]] );

        INTERPOLATE_POINTS( (*new_flat_points)[neighbours[north_pole]
                                [(neigh_index+p)%n_neighbours[north_pole]]],
                            (*new_flat_points)[path[1]],
                            (*new_flat_points)[new_ids[path[1]]],
                            current_length / north_length );
    }

    /*--- around south pole */

    neigh_index = get_neigh_index( south_pole, n_neighbours, neighbours,
                                   path[path_size-2] );
    south_length = 0.0;
    for_less( p, 0, n_neighbours[south_pole] )
    {
        south_length += distance_between_points(
                        &polygons->points[neighbours[south_pole]
                                [(neigh_index+p)%n_neighbours[south_pole]]],
                        &polygons->points[neighbours[south_pole]
                                [(neigh_index+p+1)%n_neighbours[south_pole]]] );
    }

    current_length = 0.0;
    for_less( p, 1, n_neighbours[south_pole] )
    {
        current_length += distance_between_points(
                        &polygons->points[neighbours[south_pole]
                                [(neigh_index+p-1+n_neighbours[south_pole])%
                                  n_neighbours[south_pole]]],
                        &polygons->points[neighbours[south_pole]
                                [(neigh_index+p)%n_neighbours[south_pole]]] );

        INTERPOLATE_POINTS( (*new_flat_points)[neighbours[south_pole]
                                [(neigh_index+p)%n_neighbours[south_pole]]],
                            (*new_flat_points)[new_ids[path[path_size-2]]],
                            (*new_flat_points)[path[path_size-2]],
                            current_length / south_length );
    }

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        use_new_ids = FALSE;
        for_less( vertex, 0, size )
        {
            p1 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                               vertex)];

            if( new_ids[p1] >= 0 )
            {
                use_new_ids = is_to_left_of_meridian( polygons,
                                                      n_neighbours, neighbours,
                                                      poly, path_size, path,
                                                      new_ids );
                if( use_new_ids )
                    break;
            }
        }

        for_less( vertex, 0, size )
        {
            p1 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                               vertex)];

            if( use_new_ids && new_ids[p1] >= 0 )
                p1 = new_ids[p1];

            (*new_indices)[POINT_INDEX(polygons->end_indices,poly,vertex)] = p1;
        }
    }

    FREE( new_ids );
    FREE( path );

    delete_polygon_point_neighbours( polygons, n_neighbours,
                                     neighbours, NULL, NULL );

    return( new_n_points );
}
