
#include  <internal_volume_io.h>
#include  <bicpl.h>

private  int  get_points_of_region(
    Volume  volume,
    Real    min_value,
    Real    max_value,
    Point   *points[] );

private  void  get_convex_hull(
    int              n_points,
    Point            points[],
    polygons_struct  *polygons );

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

private  int  find_limit_plane(
    int              n_points,
    Point            points[],
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
        SUB_VECTORS( offset, points[i], *centre );
        x = -DOT_VECTORS( horizontal, offset );
        y = DOT_VECTORS( vertical, offset );

        if( x == 0.0 && y == 0.0 )
            continue;

        angle = RAD_TO_DEG * compute_clockwise_rotation( x, y ) - 180.0;
        if( angle < 0.0 )
            angle += 360.0;

        if( first || angle < best_angle )
        {
            best_angle = angle;
            best_ind = i;
            first = FALSE;
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

    if( best_angle < 90.0 - 0.1 || best_angle > 270.0 + 0.1 )
        handle_internal_error( "find_limit_plane angle" );


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
    int              v0,
    int              v1,
    int              v2 )
{
    int   n_indices;

    n_indices = NUMBER_INDICES( *polygons );

    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices + 3, DEFAULT_CHUNK_SIZE );

    SET_ARRAY_SIZE( polygons->indices, n_indices, n_indices + 3,
                    DEFAULT_CHUNK_SIZE );

    polygons->indices[n_indices] = v0;
    ++n_indices;
    polygons->indices[n_indices] = v1;
    ++n_indices;
    polygons->indices[n_indices] = v2;
    ++n_indices;

    return( polygons->n_items - 1 );
}

typedef  struct
{
    int  triangle;
    int  edge;
} queue_entry;

typedef  QUEUE_STRUCT( queue_entry )  queue_struct;

#define  ENLARGE_THRESHOLD         0.25
#define  NEW_DENSITY               0.125

private  void  add_edge_to_list(
    queue_struct                 *queue,
    hash_table_struct            *edge_table,
    polygons_struct              *polygons,
    int                          triangle,
    int                          edge )
{
    int          p0, p1, keys[2];
    queue_entry  entry;

    p0 = polygons->indices[POINT_INDEX(polygons->end_indices,triangle,edge)];
    p1 = polygons->indices[POINT_INDEX(polygons->end_indices,triangle,
                           (edge+1)%3)];

    keys[0] = MIN( p0, p1 );
    keys[1] = MAX( p0, p1 );

    if( !lookup_in_hash_table( edge_table, keys, NULL ) )
    {
        insert_in_hash_table( edge_table, keys, NULL );
        entry.triangle = triangle;
        entry.edge = edge;
        INSERT_IN_QUEUE( *queue, entry );
    }
}

private  void  get_convex_hull(
    int              n_points,
    Point            points[],
    polygons_struct  *polygons )
{
    int                          i, min_ind, ind, second_ind;
    Vector                       hinge, new_hinge, normal, new_normal, other;
    int                          *new_indices, other_index;
    int                          triangle, edge, new_triangle;
    int                          vertex[3];
    queue_entry                  entry;
    queue_struct                 queue;
    hash_table_struct            edge_table;

    initialize_polygons( polygons, WHITE, NULL );

    min_ind = 0;
    for_less( i, 0, n_points )
    {
        if( i == 0 || Point_x(points[i]) < Point_x(points[min_ind]) )
            min_ind = i;
    }

    fill_Vector( hinge, 0.0, 0.0, 1.0 );
    fill_Vector( normal, -1.0, 0.0, 0.0 );

    ind = find_limit_plane( n_points, points,
                            &points[min_ind], &hinge, &normal );

    SUB_POINTS( new_hinge, points[ind], points[min_ind] );
    CROSS_VECTORS( new_normal, hinge, new_hinge );

    second_ind = find_limit_plane( n_points, points,
                                   &points[ind], &new_hinge, &new_normal );

    if( min_ind == ind || min_ind == second_ind || ind == second_ind )
        handle_internal_error( "get_convex_hull" );

    ALLOC( new_indices, n_points );

    for_less( i, 0, n_points )
        new_indices[i] = -1;

    triangle = add_polygon( polygons,
                 get_polygon_point_index(polygons,points,new_indices,min_ind),
                 get_polygon_point_index(polygons,points,new_indices,ind),
                 get_polygon_point_index(polygons,points,new_indices,
                                         second_ind) );

    INITIALIZE_QUEUE( queue );

    initialize_hash_table( &edge_table, 2, 1000,
                           ENLARGE_THRESHOLD, NEW_DENSITY );


    for_less( i, 0, 3 )
        add_edge_to_list( &queue, &edge_table, polygons, triangle, i );

    while( !IS_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_QUEUE( queue, entry );

        triangle = entry.triangle;
        edge = entry.edge;

        for_less( i, 0, 3 )
        {
            vertex[i] = polygons->indices[
                            POINT_INDEX(polygons->end_indices,triangle,i)];
        }

        SUB_POINTS( hinge, polygons->points[vertex[edge]],
                           polygons->points[vertex[(edge+1)%3]] );
        SUB_POINTS( other, polygons->points[vertex[(edge+2)%3]],
                           polygons->points[vertex[edge]] );
        CROSS_VECTORS( normal, other, hinge );

        ind = find_limit_plane( n_points, points,
                                &polygons->points[vertex[edge]],
                                &hinge, &normal );

        other_index = get_polygon_point_index(polygons,points,new_indices,ind);
        new_triangle = add_polygon( polygons,
                               vertex[edge], other_index, vertex[(edge+1)%3] );

        for_less( i, 0, 3 )
            add_edge_to_list( &queue, &edge_table, polygons, new_triangle, i );
    }

    DELETE_QUEUE( queue );

    delete_hash_table( &edge_table );

    FREE( new_indices );

    if( polygons->n_points > 0 )
    {
        ALLOC( polygons->normals, polygons->n_points );
        compute_polygon_normals( polygons );
    }
}
