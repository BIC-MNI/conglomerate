#include  <volume_io.h>
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
    float    v_dot_n, n_dot_n, v_dot_v, v_dot_e, e_dot_e, dist;
    BOOLEAN  inside_edge0, inside_edge1, inside_edge2;
    float    x0, y0, z0, x1, y1, z1, x2, y2, z2, px, py, pz;
    float    v01x, v01y, v01z;
    float    v12x, v12y, v12z;
    float    v20x, v20y, v20z;
    float    pv0x, pv0y, pv0z;
    float    pv1x, pv1y, pv1z;
    float    pv2x, pv2y, pv2z;
    float    dx, dy, dz, nx, ny, nz, tx, ty, tz;

    x0 = Point_x( tri_points[0] );
    y0 = Point_y( tri_points[0] );
    z0 = Point_z( tri_points[0] );
    x1 = Point_x( tri_points[1] );
    y1 = Point_y( tri_points[1] );
    z1 = Point_z( tri_points[1] );
    x2 = Point_x( tri_points[2] );
    y2 = Point_y( tri_points[2] );
    z2 = Point_z( tri_points[2] );

    px = Point_x( *point );
    py = Point_y( *point );
    pz = Point_z( *point );

    v01x = x1 - x0;
    v01y = y1 - y0;
    v01z = z1 - z0;
    v12x = x2 - x1;
    v12y = y2 - y1;
    v12z = z2 - z1;
    v20x = x0 - x2;
    v20y = y0 - y2;
    v20z = z0 - z2;

    nx = v20y * v01z - v20z * v01y;
    ny = v20z * v01x - v20x * v01z;
    nz = v20x * v01y - v20y * v01x;

    if( nx == 0.0f && ny == 0.0f && nz == 0.0f )
    {
        dx = px - x0;
        dy = py - y0;
        dz = pz - z0;
        return( (Real) (dx * dx + dy * dy + dz * dz) );
    }

    pv0x = px - x0;
    pv0y = py - y0;
    pv0z = pz - z0;
    pv1x = px - x1;
    pv1y = py - y1;
    pv1z = pz - z1;
    pv2x = px - x2;
    pv2y = py - y2;
    pv2z = pz - z2;

    tx = ny * v01z - nz * v01y;
    ty = nz * v01x - nx * v01z;
    tz = nx * v01y - ny * v01x;
    inside_edge0 = (tx * pv0x + ty * pv0y + tz * pv0z) >= 0.0f;

    tx = ny * v12z - nz * v12y;
    ty = nz * v12x - nx * v12z;
    tz = nx * v12y - ny * v12x;
    inside_edge1 = (tx * pv1x + ty * pv1y + tz * pv1z) >= 0.0f;

    tx = ny * v20z - nz * v20y;
    ty = nz * v20x - nx * v20z;
    tz = nx * v20y - ny * v20x;
    inside_edge2 = (tx * pv2x + ty * pv2y + tz * pv2z) >= 0.0f;

    if( inside_edge0 && inside_edge1 && inside_edge2 )
    {
        v_dot_n = pv0x * nx + pv0y * ny + pv0z * nz;
        n_dot_n = nx * nx + ny * ny + nz * nz;
        dist = v_dot_n * v_dot_n / n_dot_n;
    }
    else if( inside_edge0 && !inside_edge1 && !inside_edge2 )
    {
        dx = px - x2;
        dy = py - y2;
        dz = pz - z2;
        dist = dx * dx + dy * dy + dz * dz;
    }
    else if( !inside_edge0 && inside_edge1 && !inside_edge2 )
    {
        dx = px - x0;
        dy = py - y0;
        dz = pz - z0;
        dist = dx * dx + dy * dy + dz * dz;
    }
    else if( !inside_edge0 && !inside_edge1 && inside_edge2 )
    {
        dx = px - x1;
        dy = py - y1;
        dz = pz - z1;
        dist = dx * dx + dy * dy + dz * dz;
    }
    else
    {
        if( inside_edge0 && inside_edge1 && !inside_edge2 )
        {
            v_dot_v = pv2x * pv2x + pv2y * pv2y + pv2z * pv2z;
            v_dot_e = pv2x * v20x + pv2y * v20y + pv2z * v20z;
            e_dot_e = v20x * v20x + v20y * v20y + v20z * v20z;
        }
        else if( inside_edge0 && !inside_edge1 && inside_edge2 )
        {
            v_dot_v = pv1x * pv1x + pv1y * pv1y + pv1z * pv1z;
            v_dot_e = pv1x * v12x + pv1y * v12y + pv1z * v12z;
            e_dot_e = v12x * v12x + v12y * v12y + v12z * v12z;
        }
        else
        {
            v_dot_v = pv0x * pv0x + pv0y * pv0y + pv0z * pv0z;
            v_dot_e = pv0x * v01x + pv0y * v01y + pv0z * v01z;
            e_dot_e = v01x * v01x + v01y * v01y + v01z * v01z;
        }

        dist = v_dot_v - v_dot_e * v_dot_e / e_dot_e;
    }

    if( dist < 0.0f )
        dist = 0.0f;

    return( (Real) dist );
}
