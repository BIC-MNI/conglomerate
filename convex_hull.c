
#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  TOLERANCE_DISTANCE   1.0e-3

#define  POINT_USED_IN_CONVEX_HULL  1
#define  POINT_DISCARDED            2

private  int  get_points_of_region(
    Volume  volume,
    Real    min_value,
    Real    max_value,
    Point   *points[] );

private  void  get_convex_hull(
    int              n_points,
    Point            points[],
    polygons_struct  *polygons );

private  int  get_convex_hull_2d(
    int              n_points,
    Real             x[],
    Real             y[],
    int              hull_indices[] );

private  void  usage(
    char   executable[] )
{
    char  usage_str[] = "\n\
Usage: %s input.mnc output.obj [min_value] [max_value]\n\
\n\
     Creates a polyhedron which is the convex hull of the region of the input\n\
     volume containing values between min_value and max_value, or non-zero\n\
     if not specified.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    char           *input_filename, *output_filename;
    int            sizes[N_DIMENSIONS];
    int            n_points;
    Real           min_value, max_value;
    Point          *points;
    Volume         volume;
    object_struct  *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.01, &min_value );
    (void) get_real_argument( 1.0e30, &max_value );
 
    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    n_points = get_points_of_region( volume, min_value, max_value, &points );

    object = create_object( POLYGONS );

    get_convex_hull( n_points, points, get_polygons_ptr(object) );

    check_polygons_neighbours_computed( get_polygons_ptr(object) );

    (void) output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );

    return( 0 );
}

private  int  get_points_of_region(
    Volume  volume,
    Real    min_value,
    Real    max_value,
    Point   *points[] )
{
    int        x, y, z, sizes[N_DIMENSIONS], n_inside;
    int        dx, dy, dz, tx, ty, tz, n_points;
    Real       value, xw, yw, zw, voxel[N_DIMENSIONS];
    Point      point;

    get_volume_sizes( volume, sizes );

    n_points = 0;

    for_less( x, 0, sizes[X] + 1 )
    for_less( y, 0, sizes[Y] + 1 )
    for_less( z, 0, sizes[Z] + 1 )
    {
        n_inside = 0;

        for_less( dx, 0, 2 )
        for_less( dy, 0, 2 )
        for_less( dz, 0, 2 )
        {
            tx = x - dx;
            ty = y - dy;
            tz = z - dz;

            if( tx >= 0 && tx < sizes[X] &&
                ty >= 0 && ty < sizes[Y] &&
                tz >= 0 && tz < sizes[Z] )
            {
                value = get_volume_real_value( volume, tx, ty, tz, 0, 0 );

                if( min_value <= value && value <= max_value )
                    ++n_inside;
            }
        }

        if( n_inside == 1 )
        {
            voxel[X] = (Real) x - 0.5;
            voxel[Y] = (Real) y - 0.5;
            voxel[Z] = (Real) z - 0.5;
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( point, xw, yw, zw );
            ADD_ELEMENT_TO_ARRAY( *points, n_points, point, DEFAULT_CHUNK_SIZE);
        }
    }

    return( n_points );
}

private  Real  compute_clockwise_degrees( Real x, Real y )
{
    Real   degrees;

    if( x >= -TOLERANCE_DISTANCE && x <= TOLERANCE_DISTANCE )
    {
        if( y < -TOLERANCE_DISTANCE )
            return( 90.0 );
        else if( y > TOLERANCE_DISTANCE )
            return( 270.0 );
        else
            return( 0.0 );
    }
    else if( y >= -TOLERANCE_DISTANCE && y <= TOLERANCE_DISTANCE)
    {
        if( x > 0.0 )
            return( 0.0 );
        else
            return( 180.0 );
    }
    else
    {
        degrees = - RAD_TO_DEG * (Real) atan2( (double) y, (double) x );

        if( degrees < 0.0 )
            degrees += 360.0;

        return( degrees );
    }
}

private  int  find_limit_plane(
    int              n_points,
    Point            points[],
    Smallest_int     point_flags[],
    Point            *centre,
    Vector           *hinge,
    Vector           *normal )
{
    int      i, best_ind;
    Vector   horizontal, vertical, offset;
    Real     angle, best_angle, x, y;
    BOOLEAN  first;

    best_angle = 0.0;
    best_ind = -1;

    NORMALIZE_VECTOR( horizontal, *normal );
    CROSS_VECTORS( vertical, *normal, *hinge );
    NORMALIZE_VECTOR( vertical, vertical );

    first = TRUE;

    for_less( i, 0, n_points )
    {
        if( point_flags[i] & POINT_DISCARDED )
            continue;

        SUB_VECTORS( offset, points[i], *centre );
        x = -DOT_VECTORS( horizontal, offset );
        y = DOT_VECTORS( vertical, offset );

        if( x >= -TOLERANCE_DISTANCE && x <= TOLERANCE_DISTANCE &&
            y >= -TOLERANCE_DISTANCE && y <= TOLERANCE_DISTANCE )
            continue;

        angle = compute_clockwise_degrees( x, y ) - 180.0;
        if( angle < 0.0 )
            angle += 360.0;

        if( first || angle < best_angle )
        {
            if( angle < 90.0 - 0.1 || angle > 270.0 + 0.1 )
            {
                handle_internal_error( "find_limit_plane angle" );
            }
            else
            {
                best_angle = angle;
                best_ind = i;
                first = FALSE;
            }
        }
        else if( angle == best_angle )
        {
            if( distance_between_points( centre, &points[i] ) <
                distance_between_points( centre, &points[best_ind] ) )
            {
                best_ind = i;
            }
        }
    }

    if( best_ind < 0 )
        handle_internal_error( "find_limit_plane" );

    return( best_ind );
}

private  int  get_polygon_point_index(
    polygons_struct  *polygons,
    Point            points[],
    int              new_indices[],
    int              v )
{
    if( new_indices[v] < 0 )
    {
        new_indices[v] = polygons->n_points;
        ADD_ELEMENT_TO_ARRAY( polygons->points, polygons->n_points,
                              points[v], DEFAULT_CHUNK_SIZE );
    }

    return( new_indices[v] );
}

private  int  add_polygon(
    polygons_struct  *polygons,
    int              n_vertices,
    int              vertices[] )
{
    int   i, n_indices;

    n_indices = NUMBER_INDICES( *polygons );

    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices + n_vertices, DEFAULT_CHUNK_SIZE );

    SET_ARRAY_SIZE( polygons->indices, n_indices, n_indices + n_vertices,
                    DEFAULT_CHUNK_SIZE );

    for_less( i, 0, n_vertices )
        polygons->indices[n_indices+i] = vertices[i];

    return( polygons->n_items - 1 );
}

typedef  struct
{
    int  poly;
    int  edge;
} queue_entry;

typedef struct
{
    Smallest_int  ref_count;
} edge_struct;

typedef  QUEUE_STRUCT( queue_entry )  queue_struct;

#define  ENLARGE_THRESHOLD         0.25
#define  NEW_DENSITY               0.125

private  void  get_edge_keys(
    polygons_struct              *polygons,
    int                          poly,
    int                          edge,
    int                          keys[] )
{
    int          p0, p1, size;

    size = GET_OBJECT_SIZE( *polygons, poly );

    p0 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,edge)];
    p1 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                           (edge+1)%size)];

    keys[0] = MIN( p0, p1 );
    keys[1] = MAX( p0, p1 );
}

private  void  add_edge_to_list(
    queue_struct                 *queue,
    hash_table_struct            *edge_table,
    polygons_struct              *polygons,
    int                          poly,
    int                          edge )
{
    int          keys[2];
    edge_struct  *edge_ptr;
    queue_entry  entry;

    get_edge_keys( polygons, poly, edge, keys );

    if( lookup_in_hash_table( edge_table, keys, (void **) &edge_ptr ) )
    {
        ++edge_ptr->ref_count;
    }
    else
    {
        ALLOC( edge_ptr, 1 );
        edge_ptr->ref_count = 1;
        insert_in_hash_table( edge_table, keys, edge_ptr );

        entry.poly = poly;
        entry.edge = edge;
        INSERT_IN_QUEUE( *queue, entry );
    }
}

private  int   get_plane_polygon_vertices(
    int              n_points,
    Point            points[],
    int              new_indices[],
    Smallest_int     point_flags[],
    polygons_struct  *polygons,
    int              p0,
    int              p1,
    int              p2,
    int              vertices[] )
{
    int      i, n_in_hull, n_in_plane, *plane_points, *hull_points;
    Real     plane_constant, *x, *y, dist;
    Vector   v01, v02, normal, offset, horizontal, vertical;

    SUB_POINTS( v01, polygons->points[p1], polygons->points[p0] );
    SUB_POINTS( v02, polygons->points[p2], polygons->points[p0] );
    CROSS_VECTORS( normal, v01, v02 );
    NORMALIZE_VECTOR( normal, normal );

    plane_constant = -distance_from_plane( &polygons->points[p0], &normal,
                                           0.0 );

    n_in_plane = 0;

    for_less( i, 0, n_points )
    {
        if( point_flags[i] & POINT_DISCARDED )
            continue;

        dist = distance_from_plane( &points[i], &normal, plane_constant );
        if( dist >= -TOLERANCE_DISTANCE || new_indices[i] == p0 ||
            new_indices[i] == p1 || new_indices[i] == p2 )
        {
            ADD_ELEMENT_TO_ARRAY( plane_points, n_in_plane, i, 10 );
        }
    }

    if( n_in_plane < 3 )
        handle_internal_error( "get_plane_polygon_vertices" );

    NORMALIZE_VECTOR( horizontal, v01 );
    CROSS_VECTORS( vertical, normal, horizontal );
    NORMALIZE_VECTOR( vertical, vertical );

    ALLOC( x, n_in_plane );
    ALLOC( y, n_in_plane );
    ALLOC( hull_points, n_in_plane );

    for_less( i, 0, n_in_plane )
    {
        SUB_POINTS( offset, points[plane_points[i]], points[p0] );
        x[i] = DOT_VECTORS( offset, horizontal );
        y[i] = DOT_VECTORS( offset, vertical );
        if( ! (point_flags[plane_points[i]] & POINT_USED_IN_CONVEX_HULL) )
            point_flags[plane_points[i]] |= POINT_DISCARDED;
    }

    n_in_hull = get_convex_hull_2d( n_in_plane, x, y, hull_points );

    for_less( i, 0, n_in_hull )
    {
        point_flags[plane_points[hull_points[i]]] = POINT_USED_IN_CONVEX_HULL;
        vertices[i] = get_polygon_point_index( polygons, points, new_indices,
                                               plane_points[hull_points[i]] );
    }

    FREE( hull_points );
    FREE( x );
    FREE( y );
    FREE( plane_points );

    return( n_in_hull );
}

private  void  get_convex_hull(
    int              n_points,
    Point            points[],
    polygons_struct  *polygons )
{

    int                          i, min_ind, ind, second_ind, size;
    int                          n_bad_ref_count, n_edges;
    Vector                       hinge, new_hinge, normal, new_normal;
    int                          *new_indices, other_index, keys[2];
    int                          poly, edge, new_poly;
    int                          n_vertices, *vertices;
    int                          *poly_vertices;
    Smallest_int                 *point_flags;
    queue_entry                  entry;
    queue_struct                 queue;
    edge_struct                  *edge_ptr;
    hash_table_struct            edge_table;
    hash_table_pointer           hash_ptr;

    initialize_polygons( polygons, WHITE, NULL );

    if( n_points == 0 )
        return;

    min_ind = 0;
    for_less( i, 0, n_points )
    {
        if( i == 0 || Point_x(points[i]) < Point_x(points[min_ind]) )
            min_ind = i;
        else if( Point_x(points[i]) == Point_x(points[min_ind]) &&
                 Point_y(points[i]) <  Point_y(points[min_ind]) )
            min_ind = i;
        else if( Point_x(points[i]) == Point_x(points[min_ind]) &&
                 Point_y(points[i]) == Point_y(points[min_ind]) &&
                 Point_z(points[i]) <  Point_z(points[min_ind]) )
            min_ind = i;
    }

    fill_Vector( hinge, 0.0, 0.0, 1.0 );
    fill_Vector( normal, -1.0, 0.0, 0.0 );

    ALLOC( point_flags, n_points );

    for_less( i, 0, n_points )
        point_flags[i] = FALSE;

    ind = find_limit_plane( n_points, points, point_flags,
                            &points[min_ind], &hinge, &normal );

    SUB_POINTS( new_hinge, points[ind], points[min_ind] );
    CROSS_VECTORS( new_normal, hinge, new_hinge );

    second_ind = find_limit_plane( n_points, points, point_flags,
                                   &points[ind], &new_hinge, &new_normal );

    if( min_ind == ind || min_ind == second_ind || ind == second_ind )
        handle_internal_error( "get_convex_hull" );

    ALLOC( new_indices, n_points );
    ALLOC( vertices, n_points );
    ALLOC( poly_vertices, n_points );

    for_less( i, 0, n_points )
        new_indices[i] = -1;

    n_vertices = get_plane_polygon_vertices( n_points, points, new_indices,
                 point_flags, polygons,
                 get_polygon_point_index(polygons,points,new_indices,min_ind),
                 get_polygon_point_index(polygons,points,new_indices,ind),
                 get_polygon_point_index(polygons,points,new_indices,
                                         second_ind), vertices );
   
    poly = add_polygon( polygons, n_vertices, vertices );

    INITIALIZE_QUEUE( queue );

    initialize_hash_table( &edge_table, 2, 1000,
                           ENLARGE_THRESHOLD, NEW_DENSITY );

    for_less( i, 0, n_vertices )
        add_edge_to_list( &queue, &edge_table, polygons, poly, i );

    while( !IS_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_QUEUE( queue, entry );

        poly = entry.poly;
        edge = entry.edge;

        get_edge_keys( polygons, poly, edge, keys );

        if( !lookup_in_hash_table( &edge_table, keys, (void **) &edge_ptr ) )
            handle_internal_error( "Convex hull" );

        if( edge_ptr->ref_count >= 2 )
            continue;

        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( i, 0, size )
        {
            poly_vertices[i] = polygons->indices[
                            POINT_INDEX(polygons->end_indices,poly,i)];
        }

        SUB_POINTS( hinge, polygons->points[poly_vertices[edge]],
                           polygons->points[poly_vertices[(edge+1)%size]] );
        compute_polygon_normal( polygons, poly, &normal );

        ind = find_limit_plane( n_points, points, point_flags,
                                &polygons->points[poly_vertices[edge]],
                                &hinge, &normal );

        other_index = get_polygon_point_index(polygons,points,new_indices,ind);

        n_vertices = get_plane_polygon_vertices( n_points, points, new_indices,
                 point_flags, polygons, poly_vertices[edge], other_index,
                 poly_vertices[(edge+1)%size], vertices );

        new_poly = add_polygon( polygons, n_vertices, vertices );

        for_less( i, 0, n_vertices )
            add_edge_to_list( &queue, &edge_table, polygons, new_poly, i );
    }

    DELETE_QUEUE( queue );

    initialize_hash_pointer( &hash_ptr );

    n_bad_ref_count = 0;
    n_edges = 0;
    while( get_next_hash_entry( &edge_table, &hash_ptr, (void **) &edge_ptr ) )
    {
        if( edge_ptr->ref_count != 2 )
        {
            ++n_bad_ref_count;
        }
        ++n_edges;
        FREE( edge_ptr );
    }

    delete_hash_table( &edge_table );

    if( n_bad_ref_count > 0 )
        print( "N ref counts != 2: %d/%d\n", n_bad_ref_count, n_edges );

    FREE( new_indices );
    FREE( vertices );
    FREE( poly_vertices );
    FREE( point_flags );

    if( polygons->n_points > 0 )
    {
        ALLOC( polygons->normals, polygons->n_points );
        compute_polygon_normals( polygons );
    }
}

private  int  get_convex_hull_2d(
    int              n_points,
    Real             x[],
    Real             y[],
    int              hull_indices[] )
{
    int      i, min_ind, n_in_hull, current_ind, best_ind;
    Real     dx, dy, best_dx, best_dy, current_angle, angle, best_angle;
    BOOLEAN  first;

    min_ind = -1;
    for_less( i, 0, n_points )
    {
        if( i == 0 || x[i] < x[min_ind] )
            min_ind = i;
        else if( x[i] == x[min_ind] && y[i] < y[min_ind] )
            min_ind = i;
    }

    current_angle = 90.0;
    current_ind = min_ind;

    n_in_hull = 0;

    do
    {
        if( n_in_hull >= n_points )
            handle_internal_error( "get_convex_hull_2d" );
        hull_indices[n_in_hull] = current_ind;
        ++n_in_hull;

        best_angle = 0.0;
        first = TRUE;

        for_less( i, 0, n_points )
        {
            dx = x[i] - x[current_ind];
            dy = y[i] - y[current_ind];

            if( dx >= -TOLERANCE_DISTANCE && dx <= TOLERANCE_DISTANCE &&
                dy >= -TOLERANCE_DISTANCE && dy <= TOLERANCE_DISTANCE )
                continue;

            angle = compute_clockwise_degrees( dx, dy ) - current_angle;
            if( angle <= 0.0 )
                angle += 360.0;

            if( first || angle > best_angle )
            {
                best_dx = dx;
                best_dy = dy;
                best_ind = i;
                best_angle = angle;
                first = FALSE;
            }
            else if( angle == best_angle )
            {
                if( dx * dx + dy * dy > best_dx * best_dx + best_dy * best_dy )
                {
                    best_dx = dx;
                    best_dy = dy;
                    best_ind = i;
                }
            }
        }

        current_ind = best_ind;
        current_angle = compute_clockwise_degrees( best_dx, best_dy );
    }
    while( current_ind != min_ind );

    return( n_in_hull );
}
