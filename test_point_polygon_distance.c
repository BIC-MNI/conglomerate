#include  <internal_volume_io.h>
#include  <bicpl.h>

private  int   find_polygon_point_pairs_within_distance(
    polygons_struct     *polygons,
    int                 n_points2,
    Point               points2[],
    Real                min_distance,
    Real                max_distance,
    int                 n_x_divisions,
    int                 n_y_divisions,
    int                 n_z_divisions,
    int                 *pairs_ptr[] );

private  Real  sq_triangle_point_dist(
    Point tri_points[],
    Point *point );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING                input_filename1, input_filename2;
    STRING                output_filename;
    Real                  min_distance, max_distance;
    Real                  min_distance_sq, max_distance_sq;
    File_formats          format;
    int                   p2, size;
    int                   n_x_divisions, n_y_divisions, n_z_divisions;
    int                   n_objects1, n_objects2;
    int                   n_points2, n_pairs, poly;
    Point                 *points2, poly_points[3];
    Real                  dist;
    object_struct         **objects1, **objects2;
    polygons_struct       *polygons;
    int                   *pairs;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename1 ) ||
        !get_string_argument( NULL, &input_filename2 ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &min_distance ) ||
        !get_real_argument( 0.0, &max_distance ) )
    {
        print_error( "Usage: %s  polygons.obj  points.obj  out.obj\n", argv[0] );
        print_error( "      min_dist max_dist\n" );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_x_divisions );
    (void) get_int_argument( 100, &n_y_divisions );
    (void) get_int_argument( 100, &n_z_divisions );

    if( input_graphics_file( input_filename1,
                             &format, &n_objects1, &objects1 ) != OK ||
        n_objects1 < 1 || get_object_type(objects1[0]) != POLYGONS )
    {
        print_error( "Error in polygons file: %s\n", input_filename1 );
        return( 1 );
    }

    polygons = get_polygons_ptr( objects1[0] );

    if( input_graphics_file( input_filename2,
                             &format, &n_objects2, &objects2 ) != OK )
        return( 1 );

    n_points2 = get_object_points( objects2[0], &points2 );

    if( min_distance >= 0.0 && min_distance < max_distance &&
        n_x_divisions <= 0 )
    {
        min_distance_sq = min_distance * min_distance;
        max_distance_sq = max_distance * max_distance;

        n_pairs = 0;
        pairs = NULL;
        for_less( poly, 0, polygons->n_items )
        {
            size = GET_OBJECT_SIZE( *polygons, poly );
            if( size != 3 )
                continue;

            size = get_polygon_points( polygons, poly, poly_points );

            for_less( p2, 0, n_points2 )
            {
                dist = sq_triangle_point_dist( poly_points, &points2[p2] );

                if( min_distance_sq <= dist && dist <= max_distance_sq )
                {
                    SET_ARRAY_SIZE( pairs, n_pairs, n_pairs+2,
                                    DEFAULT_CHUNK_SIZE );
                    pairs[n_pairs] = poly;
                    ++n_pairs;
                    pairs[n_pairs] = p2;
                    ++n_pairs;
                }
            }
        }
    }
    else if( min_distance >= 0.0 && min_distance < max_distance &&
             n_x_divisions > 0 )
    {
        n_pairs = find_polygon_point_pairs_within_distance(
                          polygons, n_points2, points2,
                          min_distance, max_distance,
                          n_x_divisions,
                          n_y_divisions,
                          n_z_divisions, &pairs );
        n_pairs = -1;
    }
    else
        n_pairs = -1;

    print( "N pairs: %d\n", n_pairs );

    return( 0 );
}

private  void   find_point_ranges(
    int                n_points,
    Point              points[],
    int                n_divisions[N_DIMENSIONS],
    float              min_pos[N_DIMENSIONS],
    float              max_pos[N_DIMENSIONS] )
{
    int   dim, p;

    for_less( dim, 0, N_DIMENSIONS )
    {
         min_pos[dim] = Point_coord( points[0], dim );
         max_pos[dim] = Point_coord( points[0], dim );
    }

    for_less( p, 0, n_points )
    {
        for_less( dim, 0, N_DIMENSIONS )
        {
            if( Point_coord( points[p], dim ) < min_pos[dim] )
                min_pos[dim] = Point_coord( points[p], dim );
            else if( Point_coord( points[p], dim ) > max_pos[dim] )
                max_pos[dim] = Point_coord( points[p], dim );
        }
    }

    for_less( dim, 0, N_DIMENSIONS )
    {
        max_pos[dim] = (float) n_divisions[dim] /
                        ((float) n_divisions[dim]-0.5f)*
                        (max_pos[dim] - min_pos[dim]) + min_pos[dim];
    }
}

private  int   find_polygon_point_pairs_within_distance(
    polygons_struct     *polygons,
    int                 n_points2,
    Point               points2[],
    Real                min_distance,
    Real                max_distance,
    int                 n_x_divisions,
    int                 n_y_divisions,
    int                 n_z_divisions,
    int                 *pairs_ptr[] )
{
/*
    int                n_pairs, *pairs, n_divisions[N_DIMENSIONS];
    float              min_pos1[N_DIMENSIONS], max_pos1[N_DIMENSIONS];
    float              min_pos2[N_DIMENSIONS], max_pos2[N_DIMENSIONS];

    if( n_points1 <= 0 || n_points2 <= 0 )
        return( 0 );

    n_divisions1[X] = n_x_divisions1;
    n_divisions1[Y] = n_y_divisions1;
    n_divisions1[Z] = n_z_divisions1;
    find_point_ranges( n_points1, points1, n_divisions1, min_pos1, max_pos1 );

    n_divisions2[X] = n_x_divisions2;
    n_divisions2[Y] = n_y_divisions2;
    n_divisions2[Z] = n_z_divisions2;
    find_point_ranges( n_points1, points1, n_divisions1, min_pos1, max_pos1 );

    ALLOC( n_in_x_bin1, n_x_divisions1 );
    ALLOC( x_bin1, n_x_divisions1 );
    create_x_bins( n_points1, points1, min_pos1[X], max_pos1[X],
                   n_in_x_bin1, x_bin1 );

    ALLOC( n_in_x_bin2, n_x_divisions2 );
    ALLOC( x_bin2, n_x_divisions2 );
    create_x_bins( n_points2, points2, min_pos2[X], max_pos2[X],
                   n_in_x_bin2, x_bin2 );

    for_less( dim, 0, N_DIMENSIONS )
    {
        step1[dim] = (max_pos1[dim] - min_pos1[dim]) / n_divisions1[dim];
        step2[dim] = (max_pos2[dim] - min_pos2[dim]) / n_divisions2[dim];
    }

    max_x_offset1 = CEILING( max_distance / step1[X] );
    ALLOC( x_slabs1, 2*max_x_offset1+1 );
    for_inclusive( x, -max_x_offset1, max_x_offset1 )
    {
        ALLOC2D( x_slabs1[x+max_x_offset1], n_divisions1[Y], n_divisions1[Z] );
        if( x >= 0 && x < n_divisions1[X] )
        {
            create_x_slab( n_in_x_bin1[x], x_bin1[x], points1,
                           x_slabs1[x+max_x_offset1] );
        }
    }

    max_x_offset2 = CEILING( max_distance / step2[X] );
    ALLOC( x_slabs2, 2*max_x_offset2+1 );
    for_inclusive( x, -max_x_offset2, max_x_offset2 )
    {
        ALLOC2D( x_slabs2[x+max_x_offset2], n_divisions2[Y], n_divisions2[Z] );
        if( x >= 0 && x < n_divisions2[X] )
        {
            create_x_slab( n_in_x_bin2[x], x_bin2[x], points2,
                           x_slabs1[x+max_x_offset2] );
        }
    }

    n_pairs = 0;
    pairs = NULL;

    for_less( x1, 0, n_divisions1[X] )
    {
    }

    *pairs_ptr = pairs;

    return( n_pairs );
*/
}

private  Real  sq_triangle_point_dist(
    Point tri_points[],
    Point *point )
{
    Real     v_dot_n, n_dot_n, dist, v_dot_v, v_dot_e, e_dot_e;
    Point    p0, p1, p2;
    Vector   v01, v12, v20, normal, to_p_0, to_p_1, to_p_2, towards_inside;
    BOOLEAN  inside_edge0, inside_edge1, inside_edge2;

    p0 = tri_points[0];
    p1 = tri_points[1];
    p2 = tri_points[2];
    SUB_POINTS( v01, p1, p0 );
    SUB_POINTS( v12, p2, p1 );
    SUB_POINTS( v20, p0, p2 );
    CROSS_VECTORS( normal, v20, v01 );
    if( null_Vector( &normal ) )
        return( sq_distance_between_points( point, &tri_points[0] ) );

    SUB_POINTS( to_p_0, *point, p0 );
    SUB_POINTS( to_p_1, *point, p1 );
    SUB_POINTS( to_p_2, *point, p2 );

    CROSS_VECTORS( towards_inside, normal, v01 );
    inside_edge0 = DOT_VECTORS( towards_inside, to_p_0 ) >= 0.0;
    CROSS_VECTORS( towards_inside, normal, v12 );
    inside_edge1 = DOT_VECTORS( towards_inside, to_p_1 ) >= 0.0;
    CROSS_VECTORS( towards_inside, normal, v20 );
    inside_edge2 = DOT_VECTORS( towards_inside, to_p_2 ) >= 0.0;

    if( inside_edge0 && inside_edge1 && inside_edge2 )
    {
        v_dot_n = DOT_VECTORS( to_p_0, normal );
        n_dot_n = DOT_VECTORS( normal, normal );
        dist = v_dot_n * v_dot_n / n_dot_n;
    }
    else if( inside_edge0 && !inside_edge1 && !inside_edge2 )
        dist = sq_distance_between_points( point, &tri_points[2] );
    else if( !inside_edge0 && inside_edge1 && !inside_edge2 )
        dist = sq_distance_between_points( point, &tri_points[0] );
    else if( !inside_edge0 && !inside_edge1 && inside_edge2 )
        dist = sq_distance_between_points( point, &tri_points[1] );
    else
    {
        if( inside_edge0 && inside_edge1 && !inside_edge2 )
        {
            v_dot_v = DOT_VECTORS( to_p_2, to_p_2 );
            v_dot_e = DOT_VECTORS( to_p_2, v20 );
            e_dot_e = DOT_VECTORS( v20, v20 );
        }
        else if( inside_edge0 && !inside_edge1 && inside_edge2 )
        {
            v_dot_v = DOT_VECTORS( to_p_1, to_p_1 );
            v_dot_e = DOT_VECTORS( to_p_1, v12 );
            e_dot_e = DOT_VECTORS( v12, v12 );
        }
        else
        {
            v_dot_v = DOT_VECTORS( to_p_0, to_p_0 );
            v_dot_e = DOT_VECTORS( to_p_0, v01 );
            e_dot_e = DOT_VECTORS( v01, v01 );
        }

        dist = v_dot_v - v_dot_e * v_dot_e / e_dot_e;
    }

    if( dist < 0.0 )
        dist = 0.0;

    return( dist );
}
