#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <priority_queue.h>

private  void  convert_to_lines(
    polygons_struct  *polygons,
    Point            sphere_points[],
    BOOLEAN          on_sphere_flag,
    lines_struct     *lines );

private  void  embed_in_sphere(
    polygons_struct  *polygons,
    Real             radius,
    Point            sphere_points[] );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename;
    int                  n_objects, p;
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

    set_random_seed( 933482171 );

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

    ALLOC( sphere_points, polygons->n_points );

    embed_in_sphere( polygons, 1.0, sphere_points );

    {
        object_struct  *lines;

        lines = create_object( LINES );
        initialize_lines( get_lines_ptr(lines), RED );
        convert_to_lines( polygons, sphere_points, FALSE, get_lines_ptr(lines));
        (void) output_graphics_file( "lines1.obj", ASCII_FORMAT, 1, &lines );
        delete_object( lines );

        lines = create_object( LINES );
        initialize_lines( get_lines_ptr(lines), BLUE );
        convert_to_lines( polygons, sphere_points, TRUE, get_lines_ptr(lines));
        (void) output_graphics_file( "lines2.obj", ASCII_FORMAT, 1, &lines );
        delete_object( lines );
    }

    FREE( polygons->points );
    polygons->points = sphere_points;

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    delete_object_list( n_objects, object_list );

    output_alloc_to_file( NULL );

    return( 0 );
}

private  void  convert_to_lines(
    polygons_struct  *polygons,
    Point            sphere_points[],
    BOOLEAN          on_sphere_flag,
    lines_struct     *lines )
{
    int    poly, size, edge, p1, p2;
    Point  *used_points;

    if( on_sphere_flag )
        used_points = sphere_points;
    else
        used_points = polygons->points;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( edge, 0, size )
        {
            p1 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                               edge)];
            p2 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                               (edge+1)%size)];

            if( !null_Point(&sphere_points[p1]) &&
                !null_Point(&sphere_points[p2]) )
            {
                start_new_line( lines );
                add_point_to_line( lines, &used_points[p1] );
                add_point_to_line( lines, &used_points[p2] );
            }
        }
    }
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
    Smallest_int      used_flags[],
    int               start_point,
    int               prev_point,
    int               next_point,
    float             **distances )
{
    int                                     p, n, point_index;
    int                                     neigh, neigh_index, n_index;
    int                                     n_index1, n_index2, n_to_do, offset;
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

    n = 0;

    if( next_point >= 0 )
    {
        n_index1 = get_neigh_index( start_point, n_neighbours, neighbours,
                                    next_point );
        n_index2 = get_neigh_index( start_point, n_neighbours, neighbours,
                                    prev_point );

        offset = (n_index1 + 1) % n_neighbours[start_point];
        n_to_do = (n_index2 - n_index1 + n_neighbours[start_point]) %
                  n_neighbours[start_point] - 1;
    }
    else
    {
        offset = 0;
        n_to_do = n_neighbours[start_point];
    }

    for_less( n, 0, n_to_do )
    {
        neigh_index = (n + offset) % n_neighbours[start_point];

        point_index = neighbours[start_point][neigh_index];

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

        if( !used_flags[point_index] )
        {
            entry.point_index = point_index;
            entry.n_index = (Smallest_int) n_index;
            INSERT_IN_PRIORITY_QUEUE( queue, entry, -dist );
        }
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

                    if( !used_flags[neigh] )
                    {
                        entry.point_index = neigh;
                        entry.n_index = (Smallest_int) neigh_index;
                        INSERT_IN_PRIORITY_QUEUE( queue, entry,
                                                  (Real) -new_dist );
                    }
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
    int              prev_neigh,
    int              next_neigh,
    float            *distances[],
    int              *path_ptr[] )
{
    int     p, n_path, *path, current, best_neigh, n, neigh, tmp;
    int     n_index1, n_index2, offset, n_to_do, n_index;
    float   dist, best_dist;

    n_path = 0;
    path = NULL;
    ADD_ELEMENT_TO_ARRAY( path, n_path, to_point, DEFAULT_CHUNK_SIZE );

    current = to_point;

    do
    {
        if( prev_neigh >= 0 && next_neigh >= 0 )
        {
            n_index1 = get_neigh_index( current, n_neighbours, neighbours,
                                        next_neigh );
            n_index2 = get_neigh_index( current, n_neighbours, neighbours,
                                        prev_neigh );

            offset = (n_index1 + 1) % n_neighbours[current];
            n_to_do = (n_index2 - n_index1 + n_neighbours[current]) %
                      n_neighbours[current] - 1;
        }
        else
        {
            offset = 0;
            n_to_do = n_neighbours[current];
        }

        best_neigh = 0;
        best_dist = -1.0f;
        for_less( n, 0, n_to_do )
        {
            n_index = (n + offset) % n_neighbours[current];
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
            break;

        current = best_neigh;
        ADD_ELEMENT_TO_ARRAY( path, n_path, current, DEFAULT_CHUNK_SIZE );
        prev_neigh = -1;
        next_neigh = -1;
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
    float         *distances[],
    Smallest_int  used_flags[] )
{
    float  farthest_dist, best_dist, dist;
    int    p, furthest,n;

    farthest_dist = -1.0f;
    furthest = 0;
    for_less( p, 0, n_points )
    {
        if( used_flags[p] )
            continue;

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

    if( p == path_size - 1 )
        --p;

    if( p == 0 )
        handle_internal_error( "get_vertex_on_path(): the mesh is too small" );

    *vertex_index = p;

    return( path[p] );
}

private  void  assign_used_flags(
    int           path_size,
    int           path[],
    Smallest_int  used_flags[],
    Vector        *normal,
    Point         points[],
    Point         sphere_points[] )
{
    Real       total_length, current_length, angle, x, y, z;
    Vector     v1, v2, perp, vert;
    Transform  transform;
    int        p;

    total_length = 0.0;
    for_less( p, 0, path_size-1 )
    {
        total_length += distance_between_points( &points[path[p]],
                                                 &points[path[p+1]] );
    }

    for_less( p, 0, path_size )
        used_flags[path[p]] = TRUE;

    CONVERT_POINT_TO_VECTOR( v1, sphere_points[path[0]] );
    CONVERT_POINT_TO_VECTOR( v2, sphere_points[path[path_size-1]] );

    if( null_Vector(&v1) || null_Vector(&v2) )
        handle_internal_error( "null point found.\n" );

    if( normal != NULL )
        perp = *normal;
    else
    {
        CROSS_VECTORS( perp, v1, v2 );
        if( null_Vector(&perp) )
            handle_internal_error( "normal is null" );
    }

    NORMALIZE_VECTOR( perp, perp );

    CROSS_VECTORS( vert, perp, v1 );

    NORMALIZE_VECTOR( vert, vert );
    NORMALIZE_VECTOR( v1, v1 );

    x = DOT_VECTORS( v2, v1 );
    y = DOT_VECTORS( v2, vert );
    angle = 2.0 * PI - compute_clockwise_rotation( x, y );
    if( angle < 0.0 || angle > PI )
        handle_internal_error( "angle < 0.0 || 180.0" );

    current_length = 0.0;
    for_less( p, 1, path_size-1 )
    {
Real len;

        current_length += distance_between_points( &points[path[p-1]],
                                                   &points[path[p]] );
        make_rotation_about_axis( &perp, current_length / total_length * angle,
                                  &transform );

        transform_vector( &transform,
                          RVector_x(v1), RVector_y(v1), RVector_z(v1),
                          &x, &y, &z );
        if( !null_Point( &sphere_points[path[p]] ) )
            handle_internal_error( "sphere point not null" );

        len = x*x + y*y + z*z;

        if( len < 0.99 || len > 1.01 )
            print_error( "Length: %g\n", len );

        fill_Point( sphere_points[path[p]], x, y, z );
    }
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

private  int  cut_off_corners_if_needed(
    int               input_loop_length,
    int               loop[],
    Smallest_int      corner_flags[],
    int               n_neighbours[],
    int               *neighbours[] )
{
    int      p, loop_length, prev_index, next_index, n1, n2, prev, next;
    int      n_iters;
    BOOLEAN  changed;

    loop_length = input_loop_length;
    n_iters = 0;

    do
    {
        changed = FALSE;

        p = 0;
        while( p < loop_length )
        {
            if( !corner_flags[p] )
            {
                ++p;
                continue;
            }

            prev_index = (p-1+loop_length) % loop_length;
            next_index = (p+1) % loop_length;
            prev = loop[prev_index];
            next = loop[next_index];

            n1 = get_neigh_index( loop[p], n_neighbours, neighbours, prev );
            n2 = get_neigh_index( loop[p], n_neighbours, neighbours, next );

            if( n2 == (n1 + 1) % n_neighbours[loop[p]] )
            {
                changed = TRUE;
                print( "Deleting corner: %d\n", loop[p] );

                corner_flags[prev_index] = TRUE;
                corner_flags[next_index] = TRUE;

                DELETE_ELEMENT_FROM_ARRAY( loop, loop_length, p,
                                           DEFAULT_CHUNK_SIZE );
                ++loop_length;
                DELETE_ELEMENT_FROM_ARRAY( corner_flags, loop_length, p,
                                           DEFAULT_CHUNK_SIZE );

                if( p > 0 )
                    --p;
            }
            else
                ++p;
        }

        ++n_iters;
    }
    while( changed );

    if( n_iters > 1 )
        print( "N iterations cutting corners: %d\n", n_iters );

    return( loop_length );
}

static  int  depth = 0;
static  int  max_depth = -1;

private  void  position_patch(
    int               n_points,
    int               loop_length,
    int               loop[],
    Smallest_int      corner_flags[],
    int               n_neighbours[],
    int               *neighbours[],
    Smallest_int      used_flags[],
    float             **distances,
    Point             points[],
    Point             sphere_points[] )
{
    int               attempt, n_attempts, ind1, ind2, tmp, i, p;
    int               *new_loop, new_loop_size, path_size, *path;
    Smallest_int      *new_corner_flags;

    if( max_depth >= 0 && depth >= max_depth )
        return;

    if( loop_length <= 3 )
        return;

    loop_length = cut_off_corners_if_needed( loop_length, loop, corner_flags,
                                             n_neighbours, neighbours );

    if( loop_length <= 3 )
        return;

/*
for_less( p, 0, loop_length )
{
    if( corner_flags[p] )
        print( "%g %g %g\n",
               RPoint_x(sphere_points[loop[p]]),
               RPoint_y(sphere_points[loop[p]]),
               RPoint_z(sphere_points[loop[p]]) );
}
print( "\n" );
*/

    n_attempts = 100;

    for_less( attempt, 0, n_attempts )
    {
        ind1 = get_random_int( loop_length );
        do
        {
            ind2 = get_random_int( loop_length );
        }
        while( ind2 == ind1 );

        if( ind1 > ind2 )
        {
            tmp = ind1;
            ind1 = ind2;
            ind2 = tmp;
        }

        for_less( i, ind1+1, ind2 )
            if( corner_flags[i] )
                break;

        if( i >= ind2 )
            continue;

        for_less( i, ind2+1, ind1 + loop_length )
            if( corner_flags[i % loop_length] )
                break;

        if( i >= ind1 + loop_length )
            continue;

        calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                                   used_flags, loop[ind1],
                                   loop[(ind1-1+loop_length)%loop_length],
                                   loop[(ind1+1)%loop_length], distances );

        path_size = create_path_between_points( n_points, points,
                           n_neighbours, neighbours, loop[ind1], loop[ind2],
                           loop[(ind2-1+loop_length)%loop_length],
                           loop[(ind2+1)%loop_length], distances, &path );

        if( path_size == 1 )
        {
            FREE( path );
        }
        else
            break;
    }

    if( attempt >= n_attempts )
    {
        print( "\nToo many attempts.\n" );
        for_less( p, 0, loop_length )
        {
            if( corner_flags[p] )
                print( "###   " );
            else
                print( "      " );

            print( "%g %g %g\n",
                       RPoint_x(sphere_points[loop[p]]),
                       RPoint_y(sphere_points[loop[p]]),
                       RPoint_z(sphere_points[loop[p]]) );
        }
        print( "\n" );
        return;
    }

    assign_used_flags( path_size, path, used_flags, NULL,
                       points, sphere_points );

    new_loop_size = 0;
    new_loop = NULL;

    add_to_loop( &new_loop_size, &new_loop, ind1+1, loop, FALSE, FALSE );
    add_to_loop( &new_loop_size, &new_loop, path_size, path, FALSE, FALSE );
    add_to_loop( &new_loop_size, &new_loop, loop_length - ind2, &loop[ind2],
                 FALSE, FALSE );

    ALLOC( new_corner_flags, new_loop_size );
    for_less( p, 0, new_loop_size )
        new_corner_flags[p] = FALSE;

    for_less( p, 0, ind1 )
        new_corner_flags[p] = corner_flags[p];

    new_corner_flags[ind1] = TRUE;

    new_corner_flags[ind1 + path_size-1] = TRUE;

    for_less( p, 1, loop_length - ind2 )
        new_corner_flags[ind1+path_size-1+p] = corner_flags[ind2 + p];

    ++depth;
    position_patch( n_points, new_loop_size, new_loop, new_corner_flags,
                    n_neighbours, neighbours, used_flags, distances,
                    points, sphere_points );
    --depth;

    FREE( new_loop );
    FREE( new_corner_flags );

    new_loop_size = 0;
    new_loop = NULL;

    add_to_loop( &new_loop_size, &new_loop, ind2-ind1+1, &loop[ind1],
                 FALSE, FALSE );
    add_to_loop( &new_loop_size, &new_loop, path_size, path, TRUE, TRUE );

    ALLOC( new_corner_flags, new_loop_size );
    for_less( p, 0, new_loop_size )
        new_corner_flags[p] = FALSE;

    for_less( p, 0, ind2-ind1+1 )
        new_corner_flags[p] = corner_flags[p+ind1];

    new_corner_flags[0] = TRUE;
    new_corner_flags[ind2-ind1] = TRUE;

    ++depth;
    position_patch( n_points, new_loop_size, new_loop, new_corner_flags,
                    n_neighbours, neighbours, used_flags, distances,
                    points, sphere_points );
    --depth;

    FREE( new_loop );
    FREE( new_corner_flags );

    FREE( path );
}

private  void  embed_in_sphere(
    polygons_struct  *polygons,
    Real             radius,
    Point            sphere_points[] )
{
    int               point, *n_neighbours, **neighbours, south_pole;
    int               p, n_points, loop_size, *loop, poly;
    float             **distances;
    float             **equator_distances;
    int               left_size, *left_path, right_size, *right_path;
    int               total_neighbours, north_pole;
    int               *long_path, long_path_size, equator_point;
    int               other_equator_point, size1, size2;
    int               *equator1_path, *equator2_path, equator_index, size;
    Real              d;
    Point             *points, centroid, triangle_points[3];
    Vector            normal, pos;
    Smallest_int      *used_flags, *corner_flags;

    depth = 0;
    if( getenv( "MAX_DEPTH" ) == NULL ||
        sscanf( getenv("MAX_DEPTH"), "%d", &max_depth ) != 1 )
        max_depth = -1;

    create_polygon_point_neighbours( polygons, TRUE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    n_points = polygons->n_points;
    points = polygons->points;

    for_less( p, 0, n_points )
        fill_Point( sphere_points[p], 0.0, 0.0, 0.0 );

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

    south_pole = get_farthest_point_from( n_points, n_neighbours, distances,
                                          used_flags );

    print( "NP: %g %g %g\n", RPoint_x(points[north_pole]),
                             RPoint_y(points[north_pole]),
                             RPoint_z(points[north_pole]) );
    print( "SP: %g %g %g\n", RPoint_x(points[south_pole]),
                             RPoint_y(points[south_pole]),
                             RPoint_z(points[south_pole]) );

    long_path_size = create_path_between_points( n_points, points,
                               n_neighbours, neighbours, north_pole,
                               south_pole, -1, -1, distances, &long_path );

    equator_point = get_vertex_on_path( long_path_size, long_path, points,
                                        0.5, &equator_index );

    fill_Point( sphere_points[north_pole], 0.0, 0.0, 1.0 );
    fill_Point( sphere_points[south_pole], 0.0, 0.0, -1.0 );
    fill_Point( sphere_points[equator_point], 1.0, 0.0, 0.0 );

    assign_used_flags( equator_index+1, long_path, used_flags, NULL,
                       points, sphere_points );
    assign_used_flags( long_path_size - equator_index,
                       &long_path[equator_index], used_flags, NULL,
                       points, sphere_points );

    print( "EQ: %g %g %g\n", RPoint_x(points[equator_point]),
                             RPoint_y(points[equator_point]),
                             RPoint_z(points[equator_point]) );

    for_less( p, 0, long_path_size-1 )
    {
        int p1;

        for_less( p1, p+1, long_path_size )
        {
            if( long_path[p] == long_path[p1] )
                print( "Duplicate entries in path: %d,%d / %d.\n",
                       p, p1, long_path_size );
        }
    }

    ALLOC( equator_distances, n_points );
    ALLOC( equator_distances[0], total_neighbours );
    for_less( p, 1, n_points )
        equator_distances[p] = &equator_distances[p-1][n_neighbours[p-1]];

    calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                               used_flags, equator_point, -1, -1,
                               equator_distances );

    other_equator_point = get_farthest_point_from( n_points, n_neighbours,
                                                   equator_distances,
                                                   used_flags );

    calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                               used_flags, other_equator_point, -1, -1,
                               equator_distances );

    size1 = create_path_between_points( n_points, points,
                               n_neighbours, neighbours, other_equator_point,
                               north_pole, -1, -1, equator_distances,
                               &equator1_path );

    size2 = create_path_between_points( n_points, points,
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

    fill_Point( sphere_points[other_equator_point], -1.0, 0.0, 0.0 );

    assign_used_flags( size1, equator1_path, used_flags, NULL,
                       points, sphere_points);
    assign_used_flags( size2, equator2_path, used_flags, NULL,
                       points, sphere_points);

    calc_distances_from_point( n_points, points, n_neighbours, neighbours,
                               used_flags, other_equator_point, -1, -1,
                               equator_distances );

    left_size = create_path_between_points( n_points, points,
                               n_neighbours, neighbours, other_equator_point,
                               equator_point, long_path[equator_index-1],
                               long_path[equator_index+1], equator_distances,
                               &left_path );

    if( left_size < 2 )
        handle_internal_error( "left_size < 2" );

    right_size = create_path_between_points( n_points, points,
                               n_neighbours, neighbours, other_equator_point,
                               equator_point, long_path[equator_index+1],
                               long_path[equator_index-1], equator_distances,
                               &right_path );

    FREE( equator_distances[0] );
    FREE( equator_distances );

    if( right_size < 2 )
        handle_internal_error( "right_size < 2" );

    fill_Vector( normal, 0.0, 0.0, -1.0 );
    assign_used_flags( left_size, left_path, used_flags, &normal,
                       points, sphere_points);
    fill_Vector( normal, 0.0, 0.0, 1.0 );
    assign_used_flags( right_size, right_path, used_flags, &normal,
                       points, sphere_points );

    print( "OEQ: %g %g %g\n", RPoint_x(sphere_points[other_equator_point]),
                              RPoint_y(sphere_points[other_equator_point]),
                              RPoint_z(sphere_points[other_equator_point]) );

    /*--- create loop from other_equator to north pole to equator_point */

    loop_size = 0;
    loop = NULL;

    add_to_loop( &loop_size, &loop, left_size, left_path, TRUE, FALSE );
    add_to_loop( &loop_size, &loop, size1, equator1_path, FALSE, FALSE );
    add_to_loop( &loop_size, &loop, equator_index+1, long_path, FALSE, TRUE );

    ALLOC( corner_flags, loop_size );
    for_less( point, 0, loop_size )
        corner_flags[point] = FALSE;

    corner_flags[0] = TRUE;
    corner_flags[left_size-1] = TRUE;
    corner_flags[left_size-1 + size1-1] = TRUE;

    position_patch( n_points, loop_size, loop, corner_flags,
                    n_neighbours, neighbours, used_flags, distances,
                    points, sphere_points );

    FREE( loop );
    FREE( corner_flags );

    print( "Done 1/4\n" );

    /*--- create loop from other_equator to equator_point to south_pole */

    loop_size = 0;
    loop = NULL;

    add_to_loop( &loop_size, &loop, left_size, left_path, FALSE, FALSE );
    add_to_loop( &loop_size, &loop, long_path_size - equator_index,
                 &long_path[equator_index], FALSE, FALSE );
    add_to_loop( &loop_size, &loop, size2, equator2_path, TRUE, TRUE );

    ALLOC( corner_flags, loop_size );
    for_less( point, 0, loop_size )
        corner_flags[point] = FALSE;

    corner_flags[0] = TRUE;
    corner_flags[left_size-1] = TRUE;
    corner_flags[left_size-1 + long_path_size-equator_index-1] = TRUE;

    position_patch( n_points, loop_size, loop, corner_flags,
                    n_neighbours, neighbours, used_flags, distances,
                    points, sphere_points );

    FREE( loop );
    FREE( corner_flags );

    print( "Done 2/4\n" );

    /*--- create loop from other_equator to equator_point to north */

    loop_size = 0;
    loop = NULL;

    add_to_loop( &loop_size, &loop, right_size, right_path, FALSE, FALSE );
    add_to_loop( &loop_size, &loop, equator_index+1, long_path, TRUE, FALSE );
    add_to_loop( &loop_size, &loop, size1, equator1_path, TRUE, TRUE );

    ALLOC( corner_flags, loop_size );
    for_less( point, 0, loop_size )
        corner_flags[point] = FALSE;

    corner_flags[0] = TRUE;
    corner_flags[right_size-1] = TRUE;
    corner_flags[right_size-1 + equator_index+1 -1] = TRUE;

    position_patch( n_points, loop_size, loop, corner_flags,
                    n_neighbours, neighbours, used_flags, distances,
                    points, sphere_points );

    FREE( loop );
    FREE( corner_flags );

    print( "Done 3/4\n" );

    /*--- create loop from other_equator to south_pole to equator_point */

    loop_size = 0;
    loop = NULL;

    add_to_loop( &loop_size, &loop, size2, equator2_path, FALSE, FALSE );
    add_to_loop( &loop_size, &loop, long_path_size - equator_index,
                 &long_path[equator_index], TRUE, FALSE );
    add_to_loop( &loop_size, &loop, right_size, right_path, TRUE, TRUE );

    ALLOC( corner_flags, loop_size );
    for_less( point, 0, loop_size )
        corner_flags[point] = FALSE;

    corner_flags[0] = TRUE;
    corner_flags[size2-1] = TRUE;
    corner_flags[size2-1 + long_path_size-equator_index -1] = TRUE;

    position_patch( n_points, loop_size, loop, corner_flags,
                    n_neighbours, neighbours, used_flags, distances,
                    points, sphere_points );

    FREE( loop );
    FREE( corner_flags );

    print( "Done 4/4\n" );

    /*--- free up memory */

    FREE( distances[0] );
    FREE( distances );

    FREE( long_path );
    FREE( equator1_path );
    FREE( equator2_path );
    FREE( left_path );
    FREE( right_path );

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( p, 0, size )
            triangle_points[p] = sphere_points[polygons->indices[
                    POINT_INDEX(polygons->end_indices,poly,p)]];

        if( null_Point( &triangle_points[0] ) ||
            null_Point( &triangle_points[1] ) ||
            null_Point( &triangle_points[2] ) )
            continue;

        get_points_centroid( size, triangle_points, &centroid );
        find_polygon_normal( size, triangle_points, &normal );

        CONVERT_POINT_TO_VECTOR( pos, centroid );

        NORMALIZE_VECTOR( pos, pos );
        NORMALIZE_VECTOR( normal, normal );

        d = DOT_VECTORS( pos, normal );
        if( d < 0.8 )
            print( "Error in dot_product: %g\n", d );
    }

    delete_polygon_point_neighbours( polygons, n_neighbours,
                                     neighbours, NULL, NULL );

    FREE( used_flags );
}
