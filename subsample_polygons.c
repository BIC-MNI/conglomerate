#include <internal_volume_io.h>
#include <bicpl.h>

#define DEBUG

private  void  subsample_polygons(
    polygons_struct    *polygons,
    int                new_n_points );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  output.obj  fraction\n\
\n\
     Subsamples any polygons in the file, placing output in the original file\n\
     or in a different output file.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, output_filename;
    int              i, n_objects;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons;
    int              new_n_points;
    Real             fraction;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &fraction ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type(object_list[i]) == POLYGONS )
        {
            polygons = get_polygons_ptr( object_list[i] );

            new_n_points = ROUND( fraction * (Real) polygons->n_points );
            subsample_polygons( polygons, new_n_points );

            compute_polygon_normals( get_polygons_ptr(object_list[i]) );
        }
    }

    (void) output_graphics_file( output_filename, format,
                                 n_objects, object_list );

    delete_object_list( n_objects, object_list );

    output_alloc_to_file( ".alloc_stats" );

    return( 0 );
}

#define  MUST_RECOMPUTE   -1.0
#define  CANNOT_DELETE    -2.0

private  BOOLEAN  is_convex(
    int    n_points,
    Point  points[] )
{
    int  lowest, p, prev, cur, next;
    Vector  v0, v1, cross;

    lowest = 0;
    for_less( p, 0, n_points )
    {
        if( p == 0 || Point_y(points[p]) < Point_y(points[lowest]) )
            lowest = p;
    }

    for_less( p, 1, n_points )
    {
        prev = (lowest + p - 1 + n_points) % n_points;
        cur = (lowest + p) % n_points;
        next = (lowest + p + 1) % n_points;
        SUB_POINTS( v0, points[prev], points[cur] );
        SUB_POINTS( v1, points[next], points[cur] );
        CROSS_VECTORS( cross, v1, v0 );
        if( RVector_z(cross) < 0.0 )
            return( FALSE );

        SUB_POINTS( v0, points[lowest], points[cur] );
        CROSS_VECTORS( cross, v1, v0 );
        if( RVector_z(cross) < 0.0 )
            return( FALSE );
    }

    return( TRUE );
}

private  int   get_surrounding_polygon(
    int           n_points,
    Point         points[],
    int           point_index,
    int           n_neighbours[],
    int           *neighbours[],
    Point         *neigh_points_ptr[] )
{
    int     n_neighs, current_index, first_index, current_point, next_point;
    Point   *neigh_points;

    for_less( current_index, 0, n_neighbours[point_index] )
    {
        if( neighbours[point_index][current_index] >= 0 )
            break;
    }

    if( current_index >= n_neighbours[point_index] )
        handle_internal_error( "current_index >= n_neighbours[point_index]" );

    first_index = neighbours[point_index][current_index];

    current_point = first_index;
    for_less( current_index, 0, n_neighbours[current_point] )
    {
        if( neighbours[current_point][current_index] == point_index )
            break;
    }

    if( current_index >= n_neighbours[current_point] )
        handle_internal_error( "current_index >= n_neighbours[current_point]" );

    n_neighs = 0;
    neigh_points = NULL;

    do
    {
        ADD_ELEMENT_TO_ARRAY( neigh_points, n_neighs, points[current_point],
                              DEFAULT_CHUNK_SIZE );

        do
        {
            current_index = (current_index - 1 + n_neighbours[current_point]) %
                            n_neighbours[current_point];
        }
        while( neighbours[current_point][current_index] < 0 ||
               neighbours[current_point][current_index] == point_index );

        next_point = neighbours[current_point][current_index];

        for_less( current_index, 0, n_neighbours[next_point] )
        {
            if( neighbours[next_point][current_index] == current_point )
                break;
        }

        if( current_index >= n_neighbours[next_point] )
            handle_internal_error( "can_delete n_neighbours" );

        current_point = next_point;

        if( n_neighs > n_points )
            handle_internal_error( "n_neighs > n_points" );
    }
    while( current_point != first_index );

    if( n_neighs < 3 )
        handle_internal_error( "n_neighs" );

    *neigh_points_ptr = neigh_points;

    return( n_neighs );
}

private  void   get_flat_points(
    int           n_points,
    Point         points[],
    Vector        *normal,
    Point         flat_points[] )
{
    int     n;
    Vector  hor, vert;

    create_two_orthogonal_vectors( normal, &hor, &vert );
    NORMALIZE_VECTOR( hor, hor );
    NORMALIZE_VECTOR( vert, vert );

    for_less( n, 0, n_points )
    {
        fill_Point( flat_points[n],
                    DOT_POINT_VECTOR( hor, points[n] ) -
                    DOT_POINT_VECTOR( hor, points[0] ),
                    DOT_POINT_VECTOR( vert, points[n] ) -
                    DOT_POINT_VECTOR( vert, points[0] ), 0.0 );
    }
}

private  BOOLEAN   can_delete(
    int           n_points,
    Point         flat_points[] )
{
    BOOLEAN can_delete;

    can_delete = is_convex( n_points, flat_points );

    return( can_delete );
}

private  int  delete_point(
    int           point_index,
    int           n_neighbours[],
    int           *neighbours[],
    Smallest_int  interior_flags[],
    Real          flatness[] )
{
    int    n, *save_neighbours, neigh, nn, n_valid_neighbours;
    int    n_deleted, save_n_neighs, two_neighs[2];

    ALLOC( save_neighbours, n_neighbours[point_index] );
    save_n_neighs = n_neighbours[point_index];

    n_valid_neighbours = 0;
    for_less( n, 0, save_n_neighs )
    {
        save_neighbours[n] = neighbours[point_index][n];
        if( save_neighbours[n] >= 0 )
        {
            if( n_valid_neighbours < 2 )
                two_neighs[n_valid_neighbours] = save_neighbours[n];
            ++n_valid_neighbours;
        }
    }

    if( n_valid_neighbours == 2 )
    {
        for_less( n, 0, n_neighbours[two_neighs[0]] )
        {
            if( neighbours[two_neighs[0]][n] == two_neighs[1] )
                break;
        }

        if( n < n_neighbours[two_neighs[0]] )
            n_valid_neighbours = 3;
    }

    for_less( n, 0, save_n_neighs )
    {
        neigh = save_neighbours[n];
        if( neigh < 0 )
            continue;

        for_less( nn, 0, n_neighbours[neigh] )
        {
            if( neighbours[neigh][nn] == point_index )
            {
                if( n_valid_neighbours == 2 )
                {
                    if( two_neighs[0] == neigh )
                        neighbours[neigh][nn] = two_neighs[1];
                    else if( two_neighs[1] == neigh )
                        neighbours[neigh][nn] = two_neighs[0];
                    else
                        handle_internal_error( "( two_neighs[1] == neigh )" );
                }
                else
                {
                    neighbours[neigh][nn] = -1;
                }
                break;
            }
        }
        neighbours[point_index][n] = -1;
        flatness[neigh] = MUST_RECOMPUTE;
    }

    n_neighbours[point_index] = 0;
    n_deleted = 1;

    for_less( n, 0, save_n_neighs )
    {
        neigh = save_neighbours[n];
        if( neigh < 0 || n_neighbours[neigh] == 0 || !interior_flags[neigh] )
            continue;

        n_valid_neighbours = 0;
        for_less( nn, 0, n_neighbours[neigh] )
        {
            if( neighbours[neigh][nn] >= 0 )
                ++n_valid_neighbours;
        }

        if( n_valid_neighbours < 3 )
        {
            n_deleted += delete_point( neigh, n_neighbours, neighbours,
                                       interior_flags, flatness );
        }
    }

    FREE( save_neighbours );

    return( n_deleted );
}

private  int  get_indices(
    int           point_index,
    int           neigh_index,
    int           n_neighbours[],
    int           *neighbours[],
    Smallest_int  interior_flags[],
    int           indices[] )
{
    int    i, size, current_point, current_neigh_index;
    int    next_point;

    if( neighbours[point_index][neigh_index] < 0 )
        return( 0 );

    if( !interior_flags[point_index] )
    {
        i = neigh_index + 1;
        while( i < n_neighbours[point_index] &&
               neighbours[point_index][i] < 0 )
        {
            ++i;
        }

        if( i >= n_neighbours[point_index] )
            return( 0 );
    }

    size = 0;
    current_point = point_index;
    current_neigh_index = neigh_index;

    do
    {
        indices[size] = current_point;
        ++size;

        while( neighbours[current_point][current_neigh_index] < 0 )
        {
            current_neigh_index = (current_neigh_index - 1 +
                                   n_neighbours[current_point]) %
                                     n_neighbours[current_point];
        } 
        next_point = neighbours[current_point][current_neigh_index];

        for_less( i, 0, n_neighbours[next_point] )
        {
            if( neighbours[next_point][i] == current_point )
                break;
        }

        if( i >= n_neighbours[next_point] )
            handle_internal_error( "get_indices" );

        current_neigh_index = (i - 1 + n_neighbours[next_point]) %
                              n_neighbours[next_point];
        current_point = next_point;
    }
    while( current_point != point_index );

    if( size < 3 )
    {
        handle_internal_error( "size < 3" );
    }
    
    return( size );
}

private  Real  get_flatness(
    int     n_points,
    Point   polygon_points[],
    int     point_index,
    int     n_neighbours[],
    int     *neighbours[] )
{
    int      n, n_neighs;
    Real     flatness, constant, dist;
    Point    *neigh_points, *flat_points;
    Vector   normal;

    n_neighs = get_surrounding_polygon( n_points, polygon_points, point_index,
                                        n_neighbours,
                                        neighbours, &neigh_points );

    get_plane_through_points( n_neighs, neigh_points, &normal,
                              &constant );

    ALLOC( flat_points, n_neighs );

    get_flat_points( n_neighs, neigh_points, &normal, flat_points );

    if( !can_delete( n_neighs, flat_points ) )
    {
        FREE( neigh_points );
        FREE( flat_points );
        return( CANNOT_DELETE );
    }

#define USE_SURFACE_AREA
#ifdef  USE_SURFACE_AREA
    flatness = get_polygon_2d_area( n_neighs, flat_points );
#else
    dist = DOT_POINT_VECTOR( polygon_points[point_index], normal ) + constant;

    flatness = dist * dist;

    for_less( n, 0, n_neighs )
    {
        dist = DOT_POINT_VECTOR( neigh_points[n], normal ) + constant;
        flatness += dist * dist;
    }

    flatness /= (Real) (1 + n_neighs);
#endif

    FREE( neigh_points );
    FREE( flat_points );

    return( flatness );
}

#ifdef DEBUG
private  void  test_integrity(
    int           n_points,
    int           n_neighbours[],
    int           *neighbours[],
    Smallest_int  interior_flags[] )
{
    int   p, n, n_valid_neighs, nn, neigh;

    for_less( p, 0, n_points )
    {
        if( n_neighbours[p] == 0 )
            continue;

        n_valid_neighs = 0;
        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            if( neigh < 0 )
                continue;

            for_less( nn, n+1, n_neighbours[p] )
                if( neighbours[p][nn] == neigh )
                    handle_internal_error( "test_integrity nn" );

            for_less( nn, 0, n_neighbours[neigh] )
                if( neighbours[neigh][nn] == p )
                    break;

            if( nn >= n_neighbours[neigh] )
                handle_internal_error( "test_integrity no neighbours" );

            ++n_valid_neighs;
        }

        if( n_valid_neighs < 3 &&
            (n_valid_neighs != 2 || interior_flags[p]) )
            handle_internal_error( "test_integrity number of neighbours" );
    }
}
#endif

private  void  subsample_polygons(
    polygons_struct    *polygons,
    int                desired_n_points )
{
    int              p, point_index, n, n_points_left;
    int              *n_neighbours, **neighbours;
    int              new_n_polys, *new_end_indices, *new_indices;
    int              *indices, *new_point_index, ind, size, i, min_index;
    Smallest_int     *interior_flags;  
    Point            *new_points;
    Real             *flatness;
    progress_struct  progress;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, &interior_flags, NULL );

    ALLOC( flatness, polygons->n_points );

    for_less( p, 0, polygons->n_points )
    {
        if( !interior_flags[p] )
            continue;

        flatness[p] = get_flatness( polygons->n_points, polygons->points,
                                    p, n_neighbours, neighbours );
    }

    n_points_left = polygons->n_points;

    initialize_progress_report( &progress, FALSE,
                                n_points_left - desired_n_points,
                                "Deleting Nodes" );

    while( n_points_left > desired_n_points )
    {
        point_index = -1;
        for_less( p, 0, polygons->n_points )
        {
            if( n_neighbours[p] == 0 || !interior_flags[p] )
                continue;

            if( flatness[p] == MUST_RECOMPUTE )
            {
                flatness[p] = get_flatness( polygons->n_points,
                                            polygons->points, p,
                                            n_neighbours, neighbours );
            }

            if( point_index < 0 || flatness[p] >= 0.0 &&
                flatness[p] < flatness[point_index] )
                point_index = p;
        }

        if( point_index < 0 )
            break;

        n_points_left -= delete_point( point_index, n_neighbours, neighbours,
                                       interior_flags, flatness );

#ifdef DEBUG
        test_integrity( polygons->n_points, n_neighbours, neighbours,
                        interior_flags );
#endif

        update_progress_report( &progress, polygons->n_points - n_points_left );
    }

    terminate_progress_report( &progress );

    ALLOC( new_point_index, polygons->n_points );
    ALLOC( new_points, n_points_left );
    ind = 0;
    for_less( p, 0, polygons->n_points )
    {
        if( n_neighbours[p] > 0 )
        {
            new_points[ind] = polygons->points[p];
            new_point_index[p] = ind;
            ++ind;
        }
        else
            new_point_index[p] = -1;
    }

    if( ind != n_points_left )
        handle_internal_error( "ind != n_points_left" );

    new_n_polys = 0;
    new_end_indices = NULL;
    new_indices = NULL;
    ind = 0;
    ALLOC( indices, polygons->n_points );

    for_less( p, 0, polygons->n_points )
    {
        for_less( n, 0, n_neighbours[p] )
        {
            if( neighbours[p][n] < 0 )
                continue;

            size = get_indices( p, n, n_neighbours, neighbours, interior_flags,
                                indices );

            min_index = 0;
            for_less( i, 0, size )
            {
                if( indices[i] < indices[min_index] )
                    min_index = i;
            }

            if( size == 0 || min_index != 0 )
                continue;

            ADD_ELEMENT_TO_ARRAY( new_end_indices, new_n_polys, ind + size,
                                  DEFAULT_CHUNK_SIZE );
            SET_ARRAY_SIZE( new_indices, ind, ind + size, DEFAULT_CHUNK_SIZE );

            for_less( i, 0, size )
                new_indices[ind+i] = new_point_index[indices[i]];

            ind += size;
        }
    }

    FREE( indices  );
    FREE( new_point_index  );

    delete_polygon_point_neighbours( polygons, n_neighbours,
                                     neighbours, interior_flags, NULL );

    FREE( flatness );

    polygons->n_points = n_points_left;
    FREE( polygons->points );
    polygons->points = new_points;
    REALLOC( polygons->normals, n_points_left );
    polygons->n_items = new_n_polys;
    FREE( polygons->end_indices );
    polygons->end_indices = new_end_indices;
    FREE( polygons->indices );
    polygons->indices = new_indices;
}
