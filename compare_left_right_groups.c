#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR   0.5

#define  THRESHOLD   3.0

#define  REGISTRATION_OFFSET   0.5625

int  main(
    int    argc,
    char   *argv[] )
{
    STRING             filename, output_filename, *filenames[2];
    int                i, j, ni, nj, n_objects;
    int                surf, n_surfaces[2], group, object_index;
    File_formats       format;
    object_struct      **object_list;
    polygons_struct    *polygons;
    Real               left_x, right_x, y, z, avg;
    BOOLEAN            **samples_valid;
    Real               ***samples[2], **t_stat, t;
    Real               sx, sy, sz, prob, min_t, max_t, dist;
    Point              point, ray_origin, min_range, max_range, origin;
    Vector             ray_direction;
    Volume             volume;
    int                sizes[N_DIMENSIONS];
    Transform          linear;
    General_transform  transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &ni ) ||
        !get_int_argument( 0, &nj ) )
    {
        print_error( "Usage: %s output.mnc  ni nj input1.obj input2.obj + ... \n",
                    argv[0] );
        return( 1 );
    }

    n_surfaces[0] = 0;
    n_surfaces[1] = 0;
    group = 0;

    while( get_string_argument( NULL, &filename ) )
    {
        if( equal_strings( filename, "+" ) )
        {
            ++group;
            if( group > 1 )
            {
                print_error( "Too many +\n" );
                return( 1 );
            }
            continue;
        }

        ADD_ELEMENT_TO_ARRAY( filenames[group], n_surfaces[group], filename, 1);
    }

    if( group != 1 )
    {
        print_error( "No + found\n" );
        return( 1 );
    }

    ALLOC2D( samples_valid, ni, nj );
    for_less( i, 0, ni )
        for_less( j, 0, nj )
            samples_valid[i][j] = TRUE;

    for_less( group, 0, 2 )
    {
        ALLOC3D( samples[group], ni, nj, n_surfaces[group] );

        for_less( surf, 0, n_surfaces[group] )
        {
            if( input_graphics_file( filenames[group][surf],
                                     &format, &n_objects, &object_list ) != OK )
            {
                print( "Couldn't read %s.\n", filename );
                return( 1 );
            }
            else if( n_objects != 1 ||
                     get_object_type(object_list[0]) != POLYGONS )
            {
                print( "Invalid object in file.\n" );
                return( 1 );
            }

            polygons = get_polygons_ptr(object_list[0]);
            create_polygons_bintree( polygons,
                                     ROUND( (Real) polygons->n_items *
                                            BINTREE_FACTOR ) );

            if( group == 0 && surf == 0 )
            {
                get_range_points( polygons->n_points, polygons->points,
                                  &min_range, &max_range );
            }

            for_less( i, 0, ni )
            for_less( j, 0, nj )
            {
                y = INTERPOLATE( (Real) i / (Real) (ni-1),
                                 (Real) Point_y(min_range),
                                 (Real) Point_y(max_range) );
                z = INTERPOLATE( (Real) j / (Real) (nj-1),
                                 (Real) Point_z(min_range),
                                 (Real) Point_z(max_range) );

                fill_Point( ray_origin, (Real) Point_x(max_range) + 100.0,
                            y, z );
                fill_Vector( ray_direction, -1.0, 0.0, 0.0 );

                if( intersect_ray_with_object( &ray_origin, &ray_direction,
                                               object_list[0], &object_index,
                                               &dist, NULL ) )
                {
                    GET_POINT_ON_RAY( point, ray_origin, ray_direction, dist );
                    right_x = (Real) Point_x( point ) - REGISTRATION_OFFSET;
                }
                else
                    right_x = 0.0;

                fill_Point( ray_origin, (Real) Point_x(min_range) - 100.0, y,z);
                fill_Vector( ray_direction, 1.0, 0.0, 0.0 );

                if( intersect_ray_with_object( &ray_origin, &ray_direction,
                                               object_list[0], &object_index,
                                               &dist, NULL ) )
                {
                    GET_POINT_ON_RAY( point, ray_origin, ray_direction, dist );
                    left_x = -(Real) Point_x( point ) + REGISTRATION_OFFSET;
                }
                else
                    left_x = 0.0;

                if( left_x < THRESHOLD || right_x < THRESHOLD )
                {
                    left_x = 0.0;
                    right_x = 0.0;
                    samples[group][i][j][surf] = 0.0;
                    samples_valid[i][j] = FALSE;
                }
                else
                {
                    avg = (left_x + right_x) / 2.0;

                    samples[group][i][j][surf] = (left_x - avg) / avg;
                }
            }

            print( "%d,%d:  %s\n", group, surf+1, filenames[group][surf] );

            delete_object_list( n_objects, object_list );
        }
    }

    ALLOC2D( t_stat, ni, nj );

    min_t = 0.0;
    max_t = 0.0;

    for_less( i, 0, ni )
    for_less( j, 0, nj )
    {
        if( samples_valid[i][j] )
        {
            t = compute_two_means_t_statistic( n_surfaces[0], samples[0][i][j],
                                             n_surfaces[1], samples[1][i][j] );
        }
        else
            t = 0.0;

        if( i == 0 && j == 0 || t < min_t )
            min_t = t;
        else if( i == 0 && j == 0 || t > max_t )
            max_t = t;

        t_stat[i][j] = t;
    }

    volume = create_volume( 3, XYZ_dimension_names, NC_SHORT, FALSE,
                            0.0, 10000.0 );
    sizes[0] = 2;
    sizes[1] = ni;
    sizes[2] = nj;
    set_volume_sizes( volume, sizes );
    alloc_volume_data( volume );

    print( "Min and max: %g %g\n", min_t, max_t );
    set_volume_real_range( volume, min_t, max_t );

    for_less( i, 0, ni )
    for_less( j, 0, nj )
    {
        prob = t_stat[i][j];
  
        set_volume_real_value( volume, 0, i, j, 0, 0, prob );
        set_volume_real_value( volume, 1, i, j, 0, 0, prob );
    }

    sx = ((Real) Point_x(max_range) - (Real) Point_x(min_range)) / 1.0;
    sy = ((Real) Point_y(max_range) - (Real) Point_y(min_range)) /
         (Real) (ni-1);
    sz = ((Real) Point_z(max_range) - (Real) Point_z(min_range)) /
         (Real) (nj-1);

    make_scale_transform( sx, sy, sz, &linear );
    fill_Point( origin, Point_x(min_range), Point_y(min_range),
                        Point_z(min_range) );
    set_transform_origin( &linear, &origin );

    create_linear_transform( &transform, &linear );
    set_voxel_to_world_transform( volume, &transform );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          volume, "left-right-diff\n", NULL );

    return( 0 );
}
