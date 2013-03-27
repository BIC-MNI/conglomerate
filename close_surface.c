#include  <volume_io.h>
#include  <bicpl.h>

#define  TOLERANCE  1.0e-2

#define  CONNECTIVITY   FOUR_NEIGHBOURS

#define  BINTREE_FACTOR  0.5

#define  INSIDE_CONVEX_BIT                 128
#define  JUST_INSIDE_CONVEX_HULL_BIT        64
#define  CONNECTED_TO_OUTSIDE_BIT           32
#define  LABEL_MASK         ((CONNECTED_TO_OUTSIDE_BIT)-1)

typedef struct
{
    int   voxel[VIO_N_DIMENSIONS];
    int   n_voxels;
} convex_boundary_struct;

static  int  remove_just_inside_label(
    VIO_Volume   volume,
    int      x,
    int      y,
    int      z );

static  int   label_inside_convex_hull(
    VIO_Volume           volume,
    object_struct    *object,
    int              value_to_set );

static  int  label_just_inside_convex_hull(
    VIO_Volume                  volume,
    convex_boundary_struct  *boundaries[] );

static  int  get_volume_int_value(
    VIO_Volume  volume,
    int     x,
    int     y,
    int     z );

static  VIO_BOOL  fill_inside(
    VIO_Volume   volume,
    int      x,
    int      y,
    int      z,
    int      label_to_set,
    int      *x_error,
    int      *y_error,
    int      *z_error );

static  void  label_connected_to_outside(
    VIO_Volume   volume,
    int      x,
    int      y,
    int      z );

static  void  print_convex_boundaries(
    int                     n_bound,
    convex_boundary_struct  bounds[] );

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR                  input_volume_filename, input_surface_filename;
    VIO_STR                  output_volume_filename;
    int                     i, b, n_objects, n_convex_boundaries;
    int                     close_threshold, n_to_strip, max_region_size;
    int                     x_error, y_error, z_error, label_to_set;
    int                     sizes[VIO_N_DIMENSIONS], x, y, z, value, n_voxels;
    convex_boundary_struct  *convex_boundaries;
    VIO_STR                  history;
    VIO_File_formats            format;
    VIO_Volume                  volume;
    object_struct           **objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &input_surface_filename ) ||
        !get_string_argument( "", &output_volume_filename ) )
    {
        print( "Usage: %s  in_volume.mnc  in_surface.obj  out_volume.mnc\n",
               argv[0] );
        print( "   [label_to_set] [close_threshold]\n" );
        return( 1 );
    }

    (void) get_int_argument( 1, &label_to_set );
    (void) get_int_argument( -1, &close_threshold );
    (void) get_int_argument( 0, &n_to_strip );

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[VIO_X] )
    for_less( y, 0, sizes[VIO_Y] )
    for_less( z, 0, sizes[VIO_Z] )
    {
        value = get_volume_int_value( volume, x, y, z );
        if( (value & LABEL_MASK) != value )
        {
            print( "VIO_Volume has values with large values, which would interfere\n" ); 
            print( "with this program using the upper bits of volume as flags.\n" );
            return( 1 );
        }
    }

    if( input_graphics_file( input_surface_filename,
                             &format, &n_objects, &objects ) != VIO_OK )
        return( 1 );

    if( n_objects != 1 || get_object_type( objects[0] ) != POLYGONS )
    {
        print( "No objects in %s.\n", input_surface_filename);
        return( 1 );
    }

    print( "Filling inside convex hull\n" );
    n_voxels = label_inside_convex_hull( volume, objects[0],
                                         INSIDE_CONVEX_BIT );
    print( "   labeled %d voxels out of %d.\n", n_voxels,
           sizes[VIO_X] * sizes[VIO_Y] * sizes[VIO_Z] );

    print( "Labeling voxels just inside convex hull\n" );

    n_convex_boundaries = label_just_inside_convex_hull( volume,
                                                         &convex_boundaries );

    print_convex_boundaries( n_convex_boundaries, convex_boundaries );

    for_less( i, 0, n_to_strip )
    {
        print( "Stripping a layer around convex hull [%d/%d].\n", i+1,
               n_to_strip );

        for_less( x, 0, sizes[VIO_X] )
        for_less( y, 0, sizes[VIO_Y] )
        for_less( z, 0, sizes[VIO_Z] )
        {
            value = get_volume_int_value( volume, x, y, z );
            if( (value & JUST_INSIDE_CONVEX_HULL_BIT) != 0 )
            {
                value -= JUST_INSIDE_CONVEX_HULL_BIT;
                value -= INSIDE_CONVEX_BIT;
                set_volume_real_value( volume, x, y, z, 0, 0, (VIO_Real) value );
            }
        }

        print( "Labeling voxels just inside convex hull\n" );

        if( n_convex_boundaries > 0 )
            FREE( convex_boundaries );

        n_convex_boundaries = label_just_inside_convex_hull( volume,
                                                           &convex_boundaries );

        print_convex_boundaries( n_convex_boundaries, convex_boundaries );

        max_region_size = 0;
        for_less( b, 0, n_convex_boundaries )
        {
            if( b == 0 || convex_boundaries[b].n_voxels > max_region_size )
                max_region_size = convex_boundaries[b].n_voxels;
        }

        print( "Largest region: %d\n", max_region_size );
    }

    max_region_size = 0;
    for_less( b, 0, n_convex_boundaries )
    {
        if( b == 0 || convex_boundaries[b].n_voxels > max_region_size )
            max_region_size = convex_boundaries[b].n_voxels;
    }

    print( "Largest region: %d\n", max_region_size );

    if( close_threshold < 0 )
        close_threshold = max_region_size;

    if( close_threshold > 0 )
    {
        print( "Removing just-inside regions larger than threshold (%d)\n",
               close_threshold );

        for_less( i, 0, n_convex_boundaries )
        {
            if( convex_boundaries[i].n_voxels >= close_threshold )
            {
                if( remove_just_inside_label( volume,
                        convex_boundaries[i].voxel[VIO_X],
                        convex_boundaries[i].voxel[VIO_Y],
                        convex_boundaries[i].voxel[VIO_Z] ) !=
                        convex_boundaries[i].n_voxels )
                    handle_internal_error( "n voxels" );
            }
        }

        print( "Filling inside the voxels.\n" );

        for_less( i, 0, n_convex_boundaries )
        {
            if( convex_boundaries[i].n_voxels >= close_threshold )
            {
                if( fill_inside( volume,
                        convex_boundaries[i].voxel[VIO_X],
                        convex_boundaries[i].voxel[VIO_Y],
                        convex_boundaries[i].voxel[VIO_Z], label_to_set,
                        &x_error, &y_error, &z_error ) )
                {
                    print( "----------------- possible topological hole\n" );
                    print( "%3d: %d %d %d -- leaks through to %d %d %d\n", i+1,
                           convex_boundaries[i].voxel[VIO_X],
                           convex_boundaries[i].voxel[VIO_Y],
                           convex_boundaries[i].voxel[VIO_Z],
                           x_error, y_error, z_error );
                }
            }
        }
    }

    print( "Labeling voxels connected to outside the convex hull.\n" );

    for_less( i, 0, n_convex_boundaries )
    {
        label_connected_to_outside( volume,
                                    convex_boundaries[i].voxel[VIO_X],
                                    convex_boundaries[i].voxel[VIO_Y],
                                    convex_boundaries[i].voxel[VIO_Z] );
    }

    print( "Filling in air pockets.\n" );

    n_voxels = 0;
    for_less( x, 0, sizes[VIO_X] )
    for_less( y, 0, sizes[VIO_Y] )
    for_less( z, 0, sizes[VIO_Z] )
    {
        value = get_volume_int_value( volume, x, y, z );
        if( (value & CONNECTED_TO_OUTSIDE_BIT) == 0 &&
            (value & INSIDE_CONVEX_BIT) != 0 &&
            (value & LABEL_MASK) == 0 )
        {
            value |= label_to_set;
            set_volume_real_value( volume, x, y, z, 0, 0, (VIO_Real) value );
            ++n_voxels;
        }
    }

    if( n_voxels > 0 )
        print( "   filled in %d air pocket voxels.\n", n_voxels );

    for_less( x, 0, sizes[VIO_X] )
    for_less( y, 0, sizes[VIO_Y] )
    for_less( z, 0, sizes[VIO_Z] )
    {
        value = get_volume_int_value( volume, x, y, z );
        value = value & LABEL_MASK;
        set_volume_real_value( volume, x, y, z, 0, 0, (VIO_Real) value );
    }

    history = create_string( "Inside surface labeled." );

    (void) output_volume( output_volume_filename, NC_UNSPECIFIED,
                          FALSE, 0.0, 0.0, volume, history,
                          (minc_output_options *) NULL );

    return( 0 );
}

static  int  get_volume_int_value(
    VIO_Volume  volume,
    int     x,
    int     y,
    int     z )
{
    VIO_Real  value;

    value = get_volume_real_value( volume, x, y, z, 0, 0 );

    return( VIO_ROUND( value ) );
}

static  int   label_inside_convex_hull(
    VIO_Volume           volume,
    object_struct    *object,
    int              value_to_set )
{
    VIO_BOOL              inside;
    int                  c, x, y, z, obj_index, n_set;
    int                  sizes[VIO_MAX_DIMENSIONS], n_intersects;
    int                  n_points, int_index, next_z;
    VIO_Real                 xw, yw, zw, distances[2], limits[2][3];
    VIO_Real                 voxel[VIO_MAX_DIMENSIONS], max_value, value;
    VIO_Real                 boundary_voxel[VIO_MAX_DIMENSIONS];
    VIO_Point                ray_origin, start_ray, end_ray, *points;
    VIO_Point                point_range[2];
    VIO_Point                ray_point;
    VIO_Vector               ray_direction, offset;
    VIO_Real                 **enter_dist, **exit_dist;
    polygons_struct      *polygons;
    VIO_progress_struct      progress;

    polygons = get_polygons_ptr( object );

    max_value = get_volume_real_max( volume );

    if( BINTREE_FACTOR > 0.0 )
    {
        create_polygons_bintree( polygons,
                                 VIO_ROUND( (VIO_Real) polygons->n_items *
                                        BINTREE_FACTOR) + 1);
    }

    n_points = polygons->n_points;
    points = polygons->points;

    get_range_points( n_points, points, &point_range[0], &point_range[1] );

    for_less( x, 0, 2 )
    for_less( y, 0, 2 )
    for_less( z, 0, 2 )
    {
        convert_world_to_voxel( volume,
                                (VIO_Real) Point_x(point_range[x]),
                                (VIO_Real) Point_y(point_range[y]),
                                (VIO_Real) Point_z(point_range[z]), voxel );
        for_less( c, 0, VIO_N_DIMENSIONS )
        {
            if( x == 0 && y == 0 && z == 0 || voxel[c] < limits[0][c] )
            {
                limits[0][c] = voxel[c];
            }
            if( x == 0 && y == 0 && z == 0 || voxel[c] > limits[1][c] )
            {
                limits[1][c] = voxel[c];
            }
        }

    }

    for_less( c, 0, VIO_N_DIMENSIONS )
    {
        limits[0][c] -= 100.0;
        limits[1][c] += 100.0;
    }

    get_volume_sizes( volume, sizes );

    n_set = 0;

    VIO_ALLOC2D( enter_dist, sizes[VIO_X], sizes[VIO_Y] );
    VIO_ALLOC2D( exit_dist, sizes[VIO_X], sizes[VIO_Y] );

    initialize_progress_report( &progress, FALSE, sizes[VIO_X] * sizes[VIO_Y],
                                "Testing inside" );

    for_less( x, 0, sizes[VIO_X] )
    {
        voxel[VIO_X] = (VIO_Real) x;
        for_less( y, 0, sizes[VIO_Y] )
        {
            voxel[VIO_Y] = (VIO_Real) y;

            voxel[VIO_Z] = limits[0][VIO_Z];
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( start_ray, xw, yw, zw );

            voxel[VIO_Z] = limits[1][VIO_Z];
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( end_ray, xw, yw, zw );

            ray_origin = end_ray;
            SUB_VECTORS( ray_direction, start_ray, ray_origin );
            NORMALIZE_VECTOR( ray_direction, ray_direction );

            enter_dist[x][y] = -1.0;
            exit_dist[x][y] = -1.0;

            if( intersect_ray_with_object( &ray_origin, &ray_direction,
                                           object, &obj_index,
                                           &distances[1], NULL ) == 0 )
                continue;
   
            SUB_VECTORS( offset, end_ray, start_ray );
            distances[1] = MAGNITUDE( offset ) - distances[1];
            exit_dist[x][y] = distances[1];

            ray_origin = start_ray;
            SUB_VECTORS( ray_direction, end_ray, ray_origin );
            NORMALIZE_VECTOR( ray_direction, ray_direction );

            if( intersect_ray_with_object( &ray_origin, &ray_direction,
                                           object, &obj_index,
                                           &distances[0], NULL ) == 0 )
            {
                print( "ray distance error\n" );
            }

            enter_dist[x][y] = distances[0];

            n_intersects = 2;

            inside = FALSE;
            int_index = -1;
            next_z = -1;

            for_less( z, 0, sizes[VIO_Z] )
            {
                while( next_z <= z )
                {
                    ++int_index;
                    if( int_index < n_intersects )
                    {
                        GET_POINT_ON_RAY( ray_point, ray_origin, ray_direction,
                                          distances[int_index] );
                        convert_world_to_voxel( volume, 
                                                (VIO_Real) Point_x(ray_point),
                                                (VIO_Real) Point_y(ray_point),
                                                (VIO_Real) Point_z(ray_point),
                                                boundary_voxel );
                        next_z = VIO_CEILING( boundary_voxel[VIO_Z] );
                        inside = ((int_index % 2) == 1);
                    }
                    else
                    {
                        next_z = sizes[VIO_Z];
                        inside = FALSE;
                    }
                }

                if( inside )
                {
                    value = get_volume_real_value( volume, x, y, z, 0, 0);
                    value = (VIO_Real) ((int) value | value_to_set);
                    if( value > max_value )
                        value = max_value;
                    set_volume_real_value( volume, x, y, z, 0, 0, value);
                    ++n_set;
                }
            }

            update_progress_report( &progress, x * sizes[VIO_Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    for_less( x, 1, sizes[VIO_X]-1 )
    {
        int      dx, dy, dir;
        VIO_BOOL  error;

        for_less( y, 1, sizes[VIO_Y]-1 )
        {
            error = FALSE;

            for_less( dir, 0, 4 )
            {
                switch( dir )
                {
                case 0:  dx = 1;  dy = 0;  break;
                case 1:  dx = 1;  dy = 1;  break;
                case 2:  dx = 0;  dy = 1;  break;
                case 3:  dx = -1;  dy = 1;  break;
                }

                if( enter_dist[x-dx][y-dy] >= 0.0 && enter_dist[x+dx][y+dy] >= 0.0&&
                    (enter_dist[x][y] < 0.0 || enter_dist[x][y] - TOLERANCE >
                     (enter_dist[x-dx][y-dy] + enter_dist[x+dx][y+dy])/2.0) )
                    error = TRUE;

                if( exit_dist[x-dx][y-dy] >= 0.0 && exit_dist[x+dx][y+dy] >= 0.0&&
                    (exit_dist[x][y] < 0.0 || exit_dist[x][y] + TOLERANCE <
                     (exit_dist[x-dx][y-dy] + exit_dist[x+dx][y+dy])/2.0) )
                    error = TRUE;
            }

            if( error )
            {
                print( "%d %d: ", x, y );
                handle_internal_error( "enter_dist" );
            }
        }
    }

    VIO_FREE2D( enter_dist );
    VIO_FREE2D( exit_dist );

    return( n_set );
}

static  VIO_BOOL  is_on_convex_boundary(
    VIO_Volume   volume,
    int      sizes[],
    int      x,
    int      y,
    int      z )
{
    int   i, n_dirs, *dx, *dy, *dz, tx, ty, tz, value;

    value = get_volume_int_value( volume, x, y, z );
    if( (value & LABEL_MASK) != 0 ||
        ((value & INSIDE_CONVEX_BIT) == 0 ||
         (value & JUST_INSIDE_CONVEX_HULL_BIT) != 0) )
        return( FALSE );

    n_dirs = get_3D_neighbour_directions( CONNECTIVITY, &dx, &dy, &dz );

    for_less( i, 0, n_dirs )
    {
        tx = x + dx[i];
        ty = y + dy[i];
        tz = z + dz[i];

        if( tx < 0 || tx >= sizes[VIO_X] ||
            ty < 0 || ty >= sizes[VIO_Y] ||
            tz < 0 || tz >= sizes[VIO_Z] ||
            (get_volume_int_value( volume, tx, ty, tz ) & INSIDE_CONVEX_BIT)
                                                            == 0 )
        {
            return( TRUE );
        }
    }

    return( FALSE );
}

typedef struct
{
    short  x, y, z;
} xyz_struct;

static  int  expand_convex_boundary(
    VIO_Volume   volume,
    int      sizes[],
    int      x,
    int      y,
    int      z )
{
    int                          i, n_dirs, *dx, *dy, *dz, tx, ty, tz, value;
    int                          n_voxels;
    xyz_struct                   xyz;
    QUEUE_STRUCT( xyz_struct )   queue;

    INITIALIZE_QUEUE( queue );

    value = get_volume_int_value( volume, x, y, z );
    value |= JUST_INSIDE_CONVEX_HULL_BIT;
    set_volume_real_value( volume, x, y, z, 0, 0, (VIO_Real) value );
    xyz.x = (short) x;
    xyz.y = (short) y;
    xyz.z = (short) z;

    n_dirs = get_3D_neighbour_directions( CONNECTIVITY, &dx, &dy, &dz );

    INSERT_IN_QUEUE( queue, xyz );

    n_voxels = 1;

    while( !IS_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_QUEUE( queue, xyz );

        x = (int) xyz.x;
        y = (int) xyz.y;
        z = (int) xyz.z;

        for_less( i, 0, n_dirs )
        {
            tx = x + dx[i];
            ty = y + dy[i];
            tz = z + dz[i];

            if( tx >= 0 && tx < sizes[VIO_X] &&
                ty >= 0 && ty < sizes[VIO_Y] &&
                tz >= 0 && tz < sizes[VIO_Z] &&
                is_on_convex_boundary( volume, sizes, tx, ty, tz ) )
            {
                value = get_volume_int_value( volume, tx, ty, tz );
                value |= JUST_INSIDE_CONVEX_HULL_BIT;
                set_volume_real_value( volume, tx, ty, tz, 0, 0, (VIO_Real) value );
                xyz.x = (short) tx;
                xyz.y = (short) ty;
                xyz.z = (short) tz;

                INSERT_IN_QUEUE( queue, xyz );
                ++n_voxels;
            }
        }
    }

    DELETE_QUEUE( queue );

    return( n_voxels );
}

static  int  label_just_inside_convex_hull(
    VIO_Volume                  volume,
    convex_boundary_struct  *boundaries[] )
{
    int                     sizes[VIO_N_DIMENSIONS], x, y, z, n_boundaries;
    convex_boundary_struct  bound;

    get_volume_sizes( volume, sizes );
    n_boundaries = 0;

    for_less( x, 0, sizes[VIO_X] )
    for_less( y, 0, sizes[VIO_Y] )
    for_less( z, 0, sizes[VIO_Z] )
    {
        if( is_on_convex_boundary( volume, sizes, x, y, z ) )
        {
            bound.voxel[VIO_X] = x;
            bound.voxel[VIO_Y] = y;
            bound.voxel[VIO_Z] = z;
            bound.n_voxels = expand_convex_boundary( volume, sizes, x, y, z );
            ADD_ELEMENT_TO_ARRAY( *boundaries, n_boundaries, bound,
                                  DEFAULT_CHUNK_SIZE );
        }
    }

    return( n_boundaries );
}

static  int  remove_just_inside_label(
    VIO_Volume   volume,
    int      x,
    int      y,
    int      z )
{
    int                          i, n_dirs, *dx, *dy, *dz, tx, ty, tz, value;
    int                          n_voxels, sizes[VIO_N_DIMENSIONS];
    xyz_struct                   xyz;
    QUEUE_STRUCT( xyz_struct )   queue;

    get_volume_sizes( volume, sizes );

    INITIALIZE_QUEUE( queue );

    value = get_volume_int_value( volume, x, y, z );
    value -= JUST_INSIDE_CONVEX_HULL_BIT;
    set_volume_real_value( volume, x, y, z, 0, 0, (VIO_Real) value );
    xyz.x = (short) x;
    xyz.y = (short) y;
    xyz.z = (short) z;

    n_dirs = get_3D_neighbour_directions( CONNECTIVITY, &dx, &dy, &dz );

    INSERT_IN_QUEUE( queue, xyz );

    n_voxels = 1;

    while( !IS_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_QUEUE( queue, xyz );

        x = (int) xyz.x;
        y = (int) xyz.y;
        z = (int) xyz.z;

        for_less( i, 0, n_dirs )
        {
            tx = x + dx[i];
            ty = y + dy[i];
            tz = z + dz[i];

            if( tx >= 0 && tx < sizes[VIO_X] &&
                ty >= 0 && ty < sizes[VIO_Y] &&
                tz >= 0 && tz < sizes[VIO_Z] &&
                (get_volume_int_value( volume, tx, ty, tz ) &
                    JUST_INSIDE_CONVEX_HULL_BIT) != 0 )
            {
                value = get_volume_int_value( volume, tx, ty, tz );
                value -= JUST_INSIDE_CONVEX_HULL_BIT;
                set_volume_real_value( volume, tx, ty, tz, 0, 0, (VIO_Real) value );
                xyz.x = (short) tx;
                xyz.y = (short) ty;
                xyz.z = (short) tz;

                INSERT_IN_QUEUE( queue, xyz );
                ++n_voxels;
            }
        }
    }

    DELETE_QUEUE( queue );

    return( n_voxels );
}

static  VIO_BOOL  fill_inside(
    VIO_Volume   volume,
    int      x,
    int      y,
    int      z,
    int      label_to_set,
    int      *x_error,
    int      *y_error,
    int      *z_error )
{
    VIO_BOOL                      error;
    int                          i, n_dirs, *dx, *dy, *dz, tx, ty, tz, value;
    int                          sizes[VIO_N_DIMENSIONS];
    xyz_struct                   xyz;
    QUEUE_STRUCT( xyz_struct )   queue;

    error = FALSE;

    get_volume_sizes( volume, sizes );

    INITIALIZE_QUEUE( queue );

    value = get_volume_int_value( volume, x, y, z );
    value |= label_to_set;
    set_volume_real_value( volume, x, y, z, 0, 0, (VIO_Real) value );
    xyz.x = (short) x;
    xyz.y = (short) y;
    xyz.z = (short) z;

    n_dirs = get_3D_neighbour_directions( CONNECTIVITY, &dx, &dy, &dz );

    INSERT_IN_QUEUE( queue, xyz );

    while( !IS_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_QUEUE( queue, xyz );

        x = (int) xyz.x;
        y = (int) xyz.y;
        z = (int) xyz.z;

        for_less( i, 0, n_dirs )
        {
            tx = x + dx[i];
            ty = y + dy[i];
            tz = z + dz[i];

            if( tx >= 0 && tx < sizes[VIO_X] &&
                ty >= 0 && ty < sizes[VIO_Y] &&
                tz >= 0 && tz < sizes[VIO_Z] )
            {
                value = get_volume_int_value( volume, tx, ty, tz );

                if( (value & LABEL_MASK) == 0 &&
                    (value & INSIDE_CONVEX_BIT) != 0 )
                {
                    if( (value & JUST_INSIDE_CONVEX_HULL_BIT) != 0 &&
                        !error )
                    {
                        error = TRUE;
                        *x_error = tx;
                        *y_error = ty;
                        *z_error = tz;
                    }

                    value |= label_to_set;
                    set_volume_real_value( volume, tx, ty, tz, 0, 0,
                                           (VIO_Real) value );
                    xyz.x = (short) tx;
                    xyz.y = (short) ty;
                    xyz.z = (short) tz;
    
                    INSERT_IN_QUEUE( queue, xyz );
                }
            }
        }
    }

    DELETE_QUEUE( queue );

    return( error );
}

static  void  label_connected_to_outside(
    VIO_Volume   volume,
    int      x,
    int      y,
    int      z )
{
    int                          i, n_dirs, *dx, *dy, *dz, tx, ty, tz, value;
    int                          sizes[VIO_N_DIMENSIONS];
    xyz_struct                   xyz;
    QUEUE_STRUCT( xyz_struct )   queue;

    get_volume_sizes( volume, sizes );

    INITIALIZE_QUEUE( queue );

    value = get_volume_int_value( volume, x, y, z );

    if( (value & LABEL_MASK) != 0 )
        return;

    value |= CONNECTED_TO_OUTSIDE_BIT;
    set_volume_real_value( volume, x, y, z, 0, 0, (VIO_Real) value );

    xyz.x = (short) x;
    xyz.y = (short) y;
    xyz.z = (short) z;

    n_dirs = get_3D_neighbour_directions( CONNECTIVITY, &dx, &dy, &dz );

    INSERT_IN_QUEUE( queue, xyz );

    while( !IS_QUEUE_EMPTY(queue) )
    {
        REMOVE_FROM_QUEUE( queue, xyz );

        x = (int) xyz.x;
        y = (int) xyz.y;
        z = (int) xyz.z;

        for_less( i, 0, n_dirs )
        {
            tx = x + dx[i];
            ty = y + dy[i];
            tz = z + dz[i];

            if( tx >= 0 && tx < sizes[VIO_X] &&
                ty >= 0 && ty < sizes[VIO_Y] &&
                tz >= 0 && tz < sizes[VIO_Z] )
            {
                value = get_volume_int_value( volume, tx, ty, tz );

                if( (value & LABEL_MASK) == 0 &&
                    (value & INSIDE_CONVEX_BIT) != 0 &&
                    (value & CONNECTED_TO_OUTSIDE_BIT) == 0 )
                {
                    value |= CONNECTED_TO_OUTSIDE_BIT;
                    set_volume_real_value( volume, tx, ty, tz, 0, 0,
                                           (VIO_Real) value );
                    xyz.x = (short) tx;
                    xyz.y = (short) ty;
                    xyz.z = (short) tz;
    
                    INSERT_IN_QUEUE( queue, xyz );
                }
            }
        }
    }

    DELETE_QUEUE( queue );
}

#define  N_TO_PRINT  10

static  void  print_convex_boundaries(
    int                     n_bound,
    convex_boundary_struct  bounds[] )
{
    VIO_BOOL  first;
    int      i, order, prev_max, prev_pos, max_pos, n_voxels;

    prev_max = -1;
    max_pos = -1;
    prev_max = -1;
    prev_pos = -1;

    for_less( order, 0, N_TO_PRINT )
    {
        first = TRUE;
        for_less( i, 0, n_bound )
        {
            n_voxels = bounds[i].n_voxels;
            if( order == 0 )
            {
                if( i == 0 || n_voxels > bounds[max_pos].n_voxels )
                    max_pos = i;
            }
            else
            {
                if( (first || n_voxels > bounds[max_pos].n_voxels) &&
                    (n_voxels < prev_max ||
                     n_voxels == prev_max && i > prev_pos) )
                {
                    max_pos = i;
                    first = FALSE;
                }
            }
        }

        prev_pos = max_pos;
        prev_max = bounds[max_pos].n_voxels;

        print( "%3d: %d %d %d -- %d voxels\n", order+1,
               bounds[max_pos].voxel[VIO_X],
               bounds[max_pos].voxel[VIO_Y],
               bounds[max_pos].voxel[VIO_Z],
               bounds[max_pos].n_voxels );
    }
}
