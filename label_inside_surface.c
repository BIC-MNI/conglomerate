#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *input_volume_filename, *input_surface_filename;
    char                 *output_volume_filename;
    Real                 value_to_set;
    STRING               history;
    BOOLEAN              inside;
    File_formats         format;
    Volume               volume;
    int                  c, x, y, z, n_objects, obj_index;
    int                  sizes[MAX_DIMENSIONS], n_intersects;
    int                  n_points, int_index, next_z;
    Real                 xw, yw, zw, dist, *distances, limits[2][3];
    Real                 voxel[MAX_DIMENSIONS];
    Real                 boundary_voxel[MAX_DIMENSIONS];
    Point                ray_origin, ray_dest, *points;
    Point                point_range[2];
    Point                ray_point;
    Vector               ray_direction;
    polygons_struct      *polygons;
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
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_graphics_file( input_surface_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects != 1 || get_object_type( objects[0] ) != POLYGONS )
    {
        print( "No objects in %s.\n", input_surface_filename);
        return( 1 );
    }

    polygons = get_polygons_ptr( objects[0] );

    create_polygons_bintree( polygons, polygons->n_items * BINTREE_FACTOR );

    n_points = get_object_points( objects[0], &points );
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

    for_less( x, 0, sizes[X] )
    {
        voxel[X] = (Real) x;
        for_less( y, 0, sizes[Y] )
        {
            voxel[Y] = (Real) y;

            voxel[Z] = limits[0][Z];
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( ray_origin, xw, yw, zw );

            voxel[Z] = limits[1][Z];
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( ray_dest, xw, yw, zw );
            SUB_VECTORS( ray_direction, ray_dest, ray_origin );
            NORMALIZE_VECTOR( ray_direction, ray_direction );

            n_intersects = intersect_ray_with_object(
                                      &ray_origin, &ray_direction,
                                      objects[0], &obj_index,
                                      &dist, &distances );

if( n_intersects != 0 && n_intersects != 2 )
{
    int             i, true_n_intersects;
    Real            true_dist, *true_distances;
    bintree_struct  *bintree;

    print( "n_intersects: %d\n", n_intersects );
    bintree = polygons->bintree;
    true_n_intersects = intersect_ray_with_object(
                                      &ray_origin, &ray_direction,
                                      objects[0], &obj_index,
                                      &true_dist, &true_distances );

    if( true_n_intersects == n_intersects )
    {
        for_less( i, 0, n_intersects )
        {
            if( distances[i] != true_distances[i] )
                break;
        }
    }

    if( true_n_intersects != n_intersects || true_dist != dist ||
        i != n_intersects )
    {
        handle_internal_error( "bintree" );
    }

    if( true_n_intersects > 0 )
        FREE( true_distances );
}

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
                        next_z = CEILING( boundary_voxel[Z] );
                        inside = ((int_index % 2) == 1);
                    }
                    else
                    {
                        next_z = sizes[Z];
                        inside = FALSE;
                    }
                }

                if( inside )
                    set_volume_real_value( volume, x, y, z, 0, 0, value_to_set);
            }
                           
            if( n_intersects > 0 )
                FREE( distances );
        }
    }
    
    (void) strcpy( history, "Inside surface labeled." );

    status = output_volume( output_volume_filename, NC_UNSPECIFIED,
                            FALSE, 0.0, 0.0, volume, history,
                            (minc_output_options *) NULL );

    delete_volume( volume );

    return( status != OK );
}
