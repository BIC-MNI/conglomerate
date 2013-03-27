#include  <volume_io.h>
#include  <bicpl.h>

#define  TOLERANCE  1.0e-2

#define  BINTREE_FACTOR  0.0

private  void   label_inside_convex_hull(
    VIO_Volume           volume,
    object_struct    *object,
    int              value_to_set );

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_volume_filename, *input_surface_filename;
    char                 *output_volume_filename;
    int                  n_objects;
    VIO_Real                 value_to_set;
    VIO_STR               history;
    File_formats         format;
    VIO_Volume               volume;
    object_struct        **objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &input_surface_filename ) ||
        !get_string_argument( "", &output_volume_filename ) )
    {
        print( "Usage: %s  in_volume.mnc  in_surface.obj  out_volume.mnc\n",
               argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 1.0, &value_to_set );

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    if( input_graphics_file( input_surface_filename,
                             &format, &n_objects, &objects ) != VIO_OK )
        return( 1 );

    if( n_objects != 1 || get_object_type( objects[0] ) != POLYGONS )
    {
        print( "No objects in %s.\n", input_surface_filename);
        return( 1 );
    }

    label_inside_convex_hull( volume, objects[0], (VIO_Real) value_to_set );

    (void) strcpy( history, "Inside surface labeled." );

    (void) output_volume( output_volume_filename, NC_UNSPECIFIED,
                          FALSE, 0.0, 0.0, volume, history,
                          (minc_output_options *) NULL );

    delete_volume( volume );

    return( 0 );
}

private  void   label_inside_convex_hull(
    VIO_Volume           volume,
    object_struct    *object,
    int              value_to_set )
{
    VIO_STR               history;
    VIO_BOOL              inside;
    File_formats         format;
    int                  i, j, c, x, y, z, obj_index, n_set, best;
    int                  sizes[MAX_DIMENSIONS], n_intersects, save_n_int;
    int                  n_points, int_index, next_z;
    VIO_Real                 xw, yw, zw, dist, distances[2], limits[2][3];
    VIO_Real                 voxel[MAX_DIMENSIONS], max_value, value;
    VIO_Real                 boundary_voxel[MAX_DIMENSIONS], tmp;
    VIO_Point                ray_origin, start_ray, end_ray, *points;
    VIO_Point                point_range[2];
    VIO_Point                ray_point;
    VIO_Vector               ray_direction, offset;
    VIO_Real                 **enter_dist, **exit_dist;
    polygons_struct      *polygons;

    polygons = get_polygons_ptr( object );

    max_value = get_volume_real_max( volume );

    if( BINTREE_FACTOR > 0.0 )
    {
        create_polygons_bintree( polygons,
                                 polygons->n_items * BINTREE_FACTOR + 1);
    }

    n_points = polygons->n_points;
    points = polygons->points;

    get_range_points( n_points, points, &point_range[0], &point_range[1] );

    for_less( x, 0, 2 )
    for_less( y, 0, 2 )
    for_less( z, 0, 2 )
    {
        convert_world_to_voxel( volume, Point_x(point_range[x]),
                                Point_y(point_range[y]),
                                Point_z(point_range[z]), voxel );
        for_less( c, 0, N_DIMENSIONS )
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

    for_less( c, 0, N_DIMENSIONS )
    {
        limits[0][c] -= 100.0;
        limits[1][c] += 100.0;
    }

    get_volume_sizes( volume, sizes );

    n_set = 0;

    ALLOC2D( enter_dist, sizes[X], sizes[Y] );
    ALLOC2D( exit_dist, sizes[X], sizes[Y] );

    for_less( x, 0, sizes[X] )
    {
        voxel[X] = (VIO_Real) x;
        for_less( y, 0, sizes[Y] )
        {
            voxel[Y] = (VIO_Real) y;

            voxel[Z] = limits[0][Z];
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( start_ray, xw, yw, zw );

            voxel[Z] = limits[1][Z];
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

            for_less( z, 0, sizes[Z] )
            {
                while( next_z <= z )
                {
                    ++int_index;
                    if( int_index < n_intersects )
                    {
                        GET_POINT_ON_RAY( ray_point, ray_origin, ray_direction,
                                          distances[int_index] );
                        convert_world_to_voxel( volume, 
                                                Point_x(ray_point),
                                                Point_y(ray_point),
                                                Point_z(ray_point),
                                                boundary_voxel );
                        next_z = VIO_CEILING( boundary_voxel[Z] );
                        inside = ((int_index % 2) == 1);
                    }
                    else
                    {
                        next_z = sizes[Z];
                        inside = FALSE;
                    }
                }

                if( inside )
                {
                    value = get_volume_real_value( volume, x, y, z, 0, 0);
                    value = (int) value | value_to_set;
                    if( value > max_value )
                        value = max_value;
                    set_volume_real_value( volume, x, y, z, 0, 0, value);
                    ++n_set;
                }
            }
        }
    }

    print( "Set %d out of %d\n", n_set, sizes[X] * sizes[Y] * sizes[Z] );

    for_less( x, 1, sizes[X]-1 )
    {
        int      dx, dy, dir;
        VIO_BOOL  error;

        for_less( y, 1, sizes[Y]-1 )
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

    FREE2D( enter_dist );
    FREE2D( exit_dist );
}
