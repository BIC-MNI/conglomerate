#include <internal_volume_io.h>
#include <bicpl.h>

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

    return( 0 );
}

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

private  BOOLEAN   can_delete(
    Point         points[],
    int           point_index,
    int           n_neighbours[],
    int           *neighbours[] )
{
    int     n_neighs, n;
    Point   *neigh_points, *flat_points;
    Real    constant;
    Vector  hor, vert, normal;
    BOOLEAN can_delete;

    ALLOC( neigh_points, n_neighbours[point_index] );
    n_neighs = 0;
    for_less( n, 0, n_neighbours[point_index] )
    {
        if( neighbours[point_index][n] >= 0 )
        {
            neigh_points[n_neighs] = points[neighbours[point_index][n]];
            ++n_neighs;
        }
    }
    if( n_neighs < 3 )
        handle_internal_error( "n_neighs" );

    get_plane_through_points( n_neighs, neigh_points, &normal,
                              &constant );

    ALLOC( flat_points, n_neighs );

    create_two_orthogonal_vectors( &normal, &hor, &vert );

    for_less( n, 0, n_neighs )
    {
        fill_Point( flat_points[n],
                    DOT_POINT_VECTOR( hor, neigh_points[n] ) -
                    DOT_POINT_VECTOR( hor, neigh_points[0] ),
                    DOT_POINT_VECTOR( vert, neigh_points[n] ) -
                    DOT_POINT_VECTOR( vert, neigh_points[0] ), 0.0 );
    }

    can_delete = is_convex( n_neighs, flat_points );

    FREE( neigh_points );
    FREE( flat_points );

    return( can_delete );
}

private  int  delete_point(
    int           point_index,
    int           n_neighbours[],
    int           *neighbours[],
    Smallest_int  interior_flags[],
    Real          flatness[] )
{
    int    n, *save_neighbours, neigh, n_neighs, nn;
    int    n_deleted, save_n_neighs;

    ALLOC( save_neighbours, n_neighbours[point_index] );
    save_n_neighs = n_neighbours[point_index];

    for_less( n, 0, save_n_neighs )
        save_neighbours[n] = neighbours[point_index][n];

    for_less( n, 0, save_n_neighs )
    {
        neigh = save_neighbours[n];
        if( neigh < 0 )
            continue;

        for_less( nn, 0, n_neighbours[neigh] )
        {
            if( neighbours[neigh][nn] == point_index )
            {
                neighbours[neigh][nn] = -1;
                break;
            }
        }
        neighbours[point_index][n] = -1;
    }

    n_neighbours[point_index] = 0;
    n_deleted = 1;

    for_less( n, 0, save_n_neighs )
    {
        neigh = save_neighbours[n];
        if( neigh < 0 || n_neighbours[neigh] == 0 || !interior_flags[neigh] )
            continue;

        n_neighs = 0;
        for_less( nn, 0, n_neighbours[neigh] )
        {
            if( neighbours[neigh][nn] >= 0 )
                ++n_neighs;
        }

        if( n_neighs < 3 )
        {
            if( n_neighs != 2 )
                handle_internal_error( "delete_point" );
            n_deleted += delete_point( neigh, n_neighbours, neighbours,
                                       interior_flags, flatness );
        }
        else
            flatness[neigh] = -1.0;
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

    i = neigh_index + 1;
    while( i < n_neighbours[point_index] &&
           neighbours[point_index][i] < 0 )
    {
        ++i;
    }

    if( !interior_flags[point_index] && i >= n_neighbours[point_index] )
        return( 0 );

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

private  void  subsample_polygons(
    polygons_struct    *polygons,
    int                desired_n_points )
{
    int            p, max_neighbours, point_index, n, n_points_left;
    int            n_neighs, *n_neighbours, **neighbours;
    int            new_n_polys, *new_end_indices, *new_indices;
    int            *indices, *new_point_index, ind, size, i, min_index;
    Smallest_int   *interior_flags;  
    Point          *neigh_points, *new_points;
    Vector         normal;
    Real           *flatness, constant, dist;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, &interior_flags, NULL );

    max_neighbours = 0;
    for_less( p, 0, polygons->n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[p] );

    ALLOC( neigh_points, max_neighbours );
    ALLOC( flatness, polygons->n_points );

    for_less( p, 0, polygons->n_points )
    {
        if( !interior_flags[p] )
            continue;

        for_less( n, 0, n_neighbours[p] )
            neigh_points[n] = polygons->points[neighbours[p][n]];
        get_plane_through_points( n_neighbours[p], neigh_points, &normal,
                                  &constant );
        dist = DOT_POINT_VECTOR( polygons->points[p], normal ) + constant;
        flatness[p] = FABS( dist );
    }

    n_points_left = polygons->n_points;

    while( n_points_left > desired_n_points )
    {
        point_index = -1;
        for_less( p, 0, polygons->n_points )
        {
            if( n_neighbours[p] == 0 || !interior_flags[p] )
                continue;

            if( flatness[p] < 0.0 )
            {
                n_neighs = 0;
                for_less( n, 0, n_neighbours[p] )
                {
                    if( neighbours[p][n] >= 0 )
                    {
                        neigh_points[n_neighs] =
                                 polygons->points[neighbours[p][n]];
                        ++n_neighs;
                    }
                }
                if( n_neighs < 3 )
                    handle_internal_error( "n_neighs" );

                get_plane_through_points( n_neighs, neigh_points, &normal,
                                          &constant );
                dist = DOT_POINT_VECTOR( polygons->points[p], normal )+constant;
                flatness[p] = FABS( dist );
            }

            if( point_index < 0 || flatness[p] < flatness[point_index] &&
                can_delete( polygons->points, p, n_neighbours, neighbours ) )
                point_index = p;
        }

        if( point_index < 0 )
            break;

        n_points_left -= delete_point( point_index, n_neighbours, neighbours,
                                       interior_flags, flatness );
    }

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

    FREE( neigh_points );
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
