#include  <volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

static  void  find_boundary_blocks(
    VIO_Point              *min_point,
    VIO_Real               block_size,
    int                x_size,
    int                y_size,
    int                z_size,
    bitlist_3d_struct  *on_boundary,
    object_struct      *surface,
    VIO_Real               radius_of_curvature );
static  BOOLEAN  block_within_distance(
    VIO_Point              *min_point,
    VIO_Real               block_size,
    int                x_size,
    int                y_size,
    int                z_size,
    bitlist_3d_struct  *on_boundary,
    VIO_Point              *point,
    VIO_Real               radius );

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               src_polygons_filename, dest_polygons_filename;
    VIO_File_formats         format;
    int                  n_objects, poly, obj_index, size, dim;
    int                  x_grid_size, y_grid_size, z_grid_size;
    VIO_Real                 radius_of_curvature, dist;
    object_struct        **objects;
    polygons_struct      *polygons;
    BOOLEAN              on_boundary;
    VIO_Point                centroid, sphere_centre, *points;
    VIO_Point                found_point;
    VIO_Vector               normal;
    VIO_progress_struct      progress;
    VIO_Real                 surface_area, buried_surface_area, total_surface_area;
    VIO_Real                 block_size;
    VIO_Point                min_point, max_point;
    bitlist_3d_struct    bitlist;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_polygons_filename ) ||
        !get_string_argument( NULL, &dest_polygons_filename ) ||
        !get_real_argument( 0.0, &radius_of_curvature ) ||
        !get_real_argument( 0.0, &block_size ) )
    {
        print_error(
             "Usage: %s  src_polygons  dest_polygons radius_of_curvature block_size\n",
               argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_polygons_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects < 1 || get_object_type( objects[0] ) != POLYGONS )
    {
        print( "Must specify polygons file.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr(objects[0]);

    set_polygon_per_item_colours( polygons );

    create_polygons_bintree( polygons,
                         VIO_ROUND( (VIO_Real) polygons->n_items * BINTREE_FACTOR ) );

    get_range_points( polygons->n_points, polygons->points,
                      &min_point, &max_point );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        Point_coord( min_point,dim ) -=
               (VIO_Point_coord_type) (3.0 * block_size + radius_of_curvature);
        Point_coord( max_point,dim ) +=
               (VIO_Point_coord_type) (3.0 * block_size + radius_of_curvature);
    }

    x_grid_size = VIO_ROUND( (RPoint_x(max_point) - RPoint_x(min_point)) /
                          block_size) + 1;
    y_grid_size = VIO_ROUND( (RPoint_y(max_point) - RPoint_y(min_point)) /
                          block_size) + 1;
    z_grid_size = VIO_ROUND( (RPoint_z(max_point) - RPoint_z(min_point)) /
                          block_size) + 1;

    print( "bytes: %d\n", x_grid_size * y_grid_size * z_grid_size / 8 );

    find_boundary_blocks( &min_point, block_size,
                          x_grid_size, y_grid_size, z_grid_size,
                          &bitlist, objects[0], radius_of_curvature );

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Testing Polygons for Boundary" );

    total_surface_area = 0.0;
    buried_surface_area = 0.0;

    ALLOC( points, polygons->n_points );

    for_less( poly, 0, polygons->n_items )
    {
        size = get_polygon_points( polygons, poly, points );

        get_points_centroid( size, points, &centroid );
        find_polygon_normal( size, points, &normal );

        GET_POINT_ON_RAY( sphere_centre, centroid, normal,
                          radius_of_curvature );

        dist = find_closest_point_on_object( &sphere_centre, objects[0],
                                             &obj_index, &found_point );

        on_boundary = (obj_index == poly);

        if( !on_boundary )
        {
            if( dist > radius_of_curvature * 1.0001 )
                handle_internal_error( "dist > radius_of_curvature * 1.0001" );
        }

        if( on_boundary &&
            !block_within_distance( &min_point, block_size,
                                    x_grid_size, y_grid_size, z_grid_size,
                                    &bitlist, &sphere_centre,
                                    radius_of_curvature ) )
            on_boundary = FALSE;
        
        if( on_boundary )
            polygons->colours[poly] = WHITE;
        else
            polygons->colours[poly] = RED;

        surface_area = get_polygon_surface_area( size, points );

        total_surface_area += surface_area;
        if( !on_boundary )
            buried_surface_area += surface_area;

        update_progress_report( &progress, poly+1 );
    }

    terminate_progress_report( &progress );

    print( "Percent buried: %g\n", 100.0 *
           buried_surface_area / total_surface_area );

    FREE( points );
    delete_bitlist_3d( &bitlist );

    (void) output_graphics_file( dest_polygons_filename,
                                 format, n_objects, objects );

    delete_object_list( n_objects, objects );

    output_alloc_to_file( ".alloc_debug" );

    return( 0 );
}


static  void  convert_world_to_block(
    VIO_Point       *min_point,
    VIO_Real        block_size,
    VIO_Point       *point,
    VIO_Real        *x_block,
    VIO_Real        *y_block,
    VIO_Real        *z_block )
{
    *x_block = (RPoint_x(*point) - RPoint_x(*min_point)) / block_size;
    *y_block = (RPoint_y(*point) - RPoint_y(*min_point)) / block_size;
    *z_block = (RPoint_z(*point) - RPoint_z(*min_point)) / block_size;
}

static  void  convert_block_to_world(
    VIO_Point       *min_point,
    VIO_Real        block_size,
    VIO_Real        x_block,
    VIO_Real        y_block,
    VIO_Real        z_block,
    VIO_Point       *point )
{
    fill_Point( *point,
                RPoint_x(*min_point) + x_block * block_size,
                RPoint_y(*min_point) + y_block * block_size,
                RPoint_z(*min_point) + z_block * block_size );
}

typedef  struct
{
    unsigned short x;
    unsigned short y;
    unsigned short z;
} voxel_struct;

static  void  find_boundary_blocks(
    VIO_Point              *min_point,
    VIO_Real               block_size,
    int                x_size,
    int                y_size,
    int                z_size,
    bitlist_3d_struct  *on_boundary,
    object_struct      *surface,
    VIO_Real               radius_of_curvature )
{
    int                           n_to_do, poly, size;
    polygons_struct               *polygons;
    bitlist_3d_struct             visited_flags, intersects;
    QUEUE_STRUCT( voxel_struct )  queue;
    voxel_struct                  voxel;
    int                           x, y, z, *dx, *dy, *dz, dir, n_dirs;
    VIO_Real                          x_min_real, x_max_real, y_min_real;
    VIO_Real                          y_max_real, z_min_real, z_max_real;
    int                           x_min, x_max, y_min, y_max, z_min, z_max;
    int                           nx, ny, nz;
    VIO_Point                         block_centre, *points, lower, upper;
    VIO_Point                         max_range, min_range, nearest_point;
    VIO_Vector                        offset;
    VIO_Real                          distance, dist;
    int                           max_x, max_y, max_z;
    VIO_progress_struct               progress;

    distance = radius_of_curvature;

    create_bitlist_3d( x_size, y_size, z_size, &intersects );
    polygons = get_polygons_ptr( surface );

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Scanning Polygons" );

    ALLOC( points, polygons->n_points );

    for_less( poly, 0, polygons->n_items )
    {
        size = get_polygon_points( polygons, poly, points );

        get_range_points( size, points, &min_range, &max_range );

        fill_Vector( offset, distance, distance, distance );
        SUB_POINT_VECTOR( lower, min_range, offset );
        ADD_POINT_VECTOR( upper, max_range, offset );
        convert_world_to_block( min_point, block_size, &lower,
                                &x_min_real, &y_min_real, &z_min_real );
        convert_world_to_block( min_point, block_size, &upper,
                                &x_max_real, &y_max_real, &z_max_real );

        x_min = MAX( 0, VIO_FLOOR( x_min_real ) );
        x_max = MIN( x_size-1, CEILING( x_max_real ) );
        y_min = MAX( 0, VIO_FLOOR( y_min_real ) );
        y_max = MIN( y_size-1, CEILING( y_max_real ) );
        z_min = MAX( 0, VIO_FLOOR( z_min_real ) );
        z_max = MIN( z_size-1, CEILING( z_max_real ) );

        for_inclusive( x, x_min, x_max )
        for_inclusive( y, y_min, y_max )
        for_inclusive( z, z_min, z_max )
        {
            if( get_bitlist_bit_3d( &intersects, x, y, z ) )
                continue;

            convert_block_to_world( min_point, block_size,
                                    (VIO_Real) x + 0.5,
                                    (VIO_Real) y + 0.5,
                                    (VIO_Real) z + 0.5, &block_centre );

            dist = get_point_object_distance_sq( &block_centre,
                                                 surface, poly,
                                                 &nearest_point );

            if( dist < distance * distance )
                set_bitlist_bit_3d( &intersects, x, y, z, TRUE );
        }

        update_progress_report( &progress, poly+1 );
    }

    terminate_progress_report( &progress );

    FREE( points );

    n_dirs = get_3D_neighbour_directions( EIGHT_NEIGHBOURS, &dx, &dy, &dz );

    create_bitlist_3d( x_size, y_size, z_size, on_boundary );
    create_bitlist_3d( x_size, y_size, z_size, &visited_flags );

    INITIALIZE_QUEUE( queue );
    voxel.x = 0;
    voxel.y = 0;
    voxel.z = 0;
    INSERT_IN_QUEUE( queue, voxel );

    set_bitlist_bit_3d( &visited_flags, 0, 0, 0, TRUE );
    set_bitlist_bit_3d( on_boundary, 0, 0, 0, FALSE );

    max_x = 0;
    max_y = 0;
    max_z = 0;

    initialize_progress_report( &progress, FALSE,
                                MAX3(x_size,y_size,z_size),
                                "Filling from Outside" );

    while( !IS_QUEUE_EMPTY(queue) )
    {
        REMOVE_FROM_QUEUE( queue, voxel );

        x = (int) voxel.x;
        y = (int) voxel.y;
        z = (int) voxel.z;

        if( x > max_x || y > max_y || z > max_z )
        {
            max_x = MAX( x, max_x );
            max_y = MAX( y, max_y );
            max_z = MAX( z, max_z );
            n_to_do = MAX3( x_size - max_x, y_size - max_y, z_size - max_z );
            update_progress_report( &progress,
                                    MAX3(x_size,y_size,z_size) -
                                    n_to_do + 1 );
        }

        for_less( dir, 0, n_dirs )
        {
            nx = x + dx[dir];
            ny = y + dy[dir];
            nz = z + dz[dir];

            if( nx < 0 || nx >= x_size ||
                ny < 0 || ny >= y_size ||
                nz < 0 || nz >= z_size ||
                get_bitlist_bit_3d( &visited_flags, nx, ny, nz ) )
                continue;

            set_bitlist_bit_3d( &visited_flags, nx, ny, nz, TRUE );

            if( get_bitlist_bit_3d( &intersects, nx, ny, nz ) )
                set_bitlist_bit_3d( on_boundary, nx, ny, nz, TRUE );
            else
            {
                voxel.x = nx;
                voxel.y = ny;
                voxel.z = nz;
                INSERT_IN_QUEUE( queue, voxel );
            }
        }
    }

    terminate_progress_report( &progress );

    DELETE_QUEUE( queue );

    delete_bitlist_3d( &intersects );
    delete_bitlist_3d( &visited_flags );
}

static  BOOLEAN  block_within_distance(
    VIO_Point              *min_point,
    VIO_Real               block_size,
    int                x_size,
    int                y_size,
    int                z_size,
    bitlist_3d_struct  *on_boundary,
    VIO_Point              *point,
    VIO_Real               radius )
{
    int        dim, x_min, x_max, y_min, y_max, z_min, z_max;
    int        x, y, z;
    VIO_Real       x_min_real, x_max_real, y_min_real;
    VIO_Real       y_max_real, z_min_real, z_max_real;
    VIO_Real       dist, delta;
    VIO_Vector     offset;
    VIO_Point      lower, upper, start_block, end_block;

    fill_Vector( offset, radius, radius, radius );
    SUB_POINT_VECTOR( lower, *point, offset );
    ADD_POINT_VECTOR( upper, *point, offset );
    convert_world_to_block( min_point, block_size, &lower,
                            &x_min_real, &y_min_real, &z_min_real );
    convert_world_to_block( min_point, block_size, &upper,
                            &x_max_real, &y_max_real, &z_max_real );

    x_min = MAX( 0, VIO_FLOOR( x_min_real ) );
    x_max = MIN( x_size-1, CEILING( x_max_real ) );
    y_min = MAX( 0, VIO_FLOOR( y_min_real ) );
    y_max = MIN( y_size-1, CEILING( y_max_real ) );
    z_min = MAX( 0, VIO_FLOOR( z_min_real ) );
    z_max = MIN( z_size-1, CEILING( z_max_real ) );

    for_inclusive( x, x_min, x_max )
    for_inclusive( y, y_min, y_max )
    for_inclusive( z, z_min, z_max )
    {
        if( get_bitlist_bit_3d( on_boundary, x, y, z ) )
        {
            convert_block_to_world( min_point, block_size,
                                    (VIO_Real) x,
                                    (VIO_Real) y,
                                    (VIO_Real) z, &start_block );
            convert_block_to_world( min_point, block_size,
                                    (VIO_Real) x + 1.0,
                                    (VIO_Real) y + 1.0,
                                    (VIO_Real) z + 1.0, &end_block );

            dist = 0.0;
            for_less( dim, 0, VIO_N_DIMENSIONS )
            {
                if( RPoint_coord(*point,dim) < RPoint_coord(start_block,dim) )
                {
                    delta = RPoint_coord(*point,dim) -
                            RPoint_coord(start_block,dim);
                }
                else if( RPoint_coord(*point,dim) >
                         RPoint_coord(end_block,dim) )
                {
                    delta = RPoint_coord(*point,dim) -
                            RPoint_coord(end_block,dim);
                }
                else
                    delta = 0.0;

                dist += delta * delta;
            }

            if( dist <= radius * radius )
                return( TRUE );
        }
    }

    return( FALSE );
}
