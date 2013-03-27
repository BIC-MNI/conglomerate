#include  <volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR   0.5

#define  THRESHOLD   3.0

#define  REGISTRATION_OFFSET   0.5625

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR           filename, output_filename, *filenames;
    FILE             *file;
    int              i, j, ni, nj, n_objects, n_surfaces, object_index;
    int              surf;
    VIO_File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons;
    VIO_Real             left_x, right_x, y, z, dist, avg;
    VIO_BOOL          **samples_valid;
    VIO_Real             ***samples, **means, **variance, **t_stat;
    VIO_Real             sum_x, sum_xx, s, se, min_t, max_t, y_pos, z_pos;
    VIO_Real             sx, sy, sz, prob;
    VIO_Point            point, ray_origin, min_range, max_range, origin;
    VIO_Vector           ray_direction;
    VIO_Volume           volume;
    int              sizes[VIO_N_DIMENSIONS];
    VIO_Transform        linear;
    VIO_General_transform  transform;
    VIO_BOOL            outputting_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &ni ) ||
        !get_int_argument( 0, &nj ) )
    {
        print_error( "Usage: %s output.mnc  ni nj input1.obj input2.obj ... \n",
                    argv[0] );
        print_error( "or: %s output.mnc  0 0 y z input1.obj input2.obj ... \n",
                    argv[0] );
        return( 1 );
    }

    outputting_flag = (ni > 0 && nj > 0);
    if( !outputting_flag )
    {
        if( !get_real_argument( 0.0, &y_pos ) ||
            !get_real_argument( 0.0, &z_pos ) )
        {
            print_error( "Usage: %s output.mnc  0 0 y z input1.obj input2.obj ... \n",
                    argv[0] );
            return( 1 );
        }

        ni = 1;
        nj = 1;
    }

    n_surfaces = 0;
    filenames = NULL;

    while( get_string_argument( NULL, &filename ) )
    {
        ADD_ELEMENT_TO_ARRAY( filenames, n_surfaces, filename, 1 );
    }

    VIO_ALLOC3D( samples, n_surfaces, ni, nj );
    VIO_ALLOC2D( samples_valid, ni, nj );

    for_less( i, 0, ni )
    for_less( j, 0, nj )
        samples_valid[i][j] = TRUE;

    if( !outputting_flag &&
        open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != VIO_OK )
    {
        return( 1 );
    }

    for_less( surf, 0, n_surfaces )
    {
        if( input_graphics_file( filenames[surf], &format, &n_objects,
                                 &object_list ) != VIO_OK )
        {
            print( "Couldn't read %s.\n", filenames[surf] );
            return( 1 );
        }
        else if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
        {
            print( "Invalid object in file.\n" );
            return( 1 );
        }
        else
        {
            polygons = get_polygons_ptr( object_list[0] );

            if( ni > 10 && nj > 10 )
                create_polygons_bintree( polygons,
                                         VIO_ROUND( (VIO_Real) polygons->n_items *
                                                .1 ) );
            else
                create_polygons_bintree( polygons,
                                         VIO_ROUND( (VIO_Real) polygons->n_items *
                                                BINTREE_FACTOR ) );

            if( surf == 0 )
            {
                get_range_points( polygons->n_points, polygons->points,
                                  &min_range, &max_range );
            }

            for_less( i, 0, ni )
            for_less( j, 0, nj )
            {
                if( outputting_flag )
                {
                    y = VIO_INTERPOLATE( (VIO_Real) i / (VIO_Real) (ni-1),
                                     (VIO_Real) Point_y(min_range),
                                     (VIO_Real) Point_y(max_range) );
                    z = VIO_INTERPOLATE( (VIO_Real) j / (VIO_Real) (nj-1),
                                     (VIO_Real) Point_z(min_range),
                                     (VIO_Real) Point_z(max_range) );
                }
                else
                {
                    y = y_pos;
                    z = z_pos;
                }

                fill_Point( ray_origin, (VIO_Real) Point_x(max_range) + 100.0,
                                        y, z );
                fill_Vector( ray_direction, -1.0, 0.0, 0.0 );

                if( intersect_ray_with_object( &ray_origin, &ray_direction,
                                               object_list[0], &object_index,
                                               &dist, NULL ) )
                {
                    GET_POINT_ON_RAY( point, ray_origin, ray_direction, dist );
                    right_x = (VIO_Real) Point_x( point ) - REGISTRATION_OFFSET;
                }
                else
                    right_x = 0.0;

                fill_Point( ray_origin, (VIO_Real) Point_x(min_range) - 100.0,
                                        y, z );
                fill_Vector( ray_direction, 1.0, 0.0, 0.0 );

                if( intersect_ray_with_object( &ray_origin, &ray_direction,
                                               object_list[0], &object_index,
                                               &dist, NULL ) )
                {
                    GET_POINT_ON_RAY( point, ray_origin, ray_direction, dist );
                    left_x = -(VIO_Real) Point_x( point ) + REGISTRATION_OFFSET;
                }
                else
                    left_x = 0.0;

                if( left_x < THRESHOLD || right_x < THRESHOLD )
                {
                    left_x = 0.0;
                    right_x = 0.0;
                    samples[surf][i][j] = 0.0;
                    samples_valid[i][j] = FALSE;
                }
                else
                {
                    avg = (left_x + right_x) / 2.0;

                    samples[surf][i][j] = (left_x - avg) / avg;
                }
            }

            if( output_real( file, samples[surf][0][0] ) != VIO_OK ||
                output_newline( file ) != VIO_OK )
                return( 1 );

            print( "%d:  %s\n", surf+1, filenames[surf] );

            delete_object_list( n_objects, object_list );
        }
    }

    VIO_ALLOC2D( means, ni, nj );
    VIO_ALLOC2D( variance, ni, nj );
    VIO_ALLOC2D( t_stat, ni, nj );

    min_t = 0.0;
    max_t = 0.0;

    for_less( i, 0, ni )
    for_less( j, 0, nj )
    {
        if( samples_valid[i][j] )
        {
            sum_x = 0.0;
            sum_xx = 0.0;

            for_less( surf, 0, n_surfaces )
            {
                sum_x += samples[surf][i][j];
                sum_xx += samples[surf][i][j] * samples[surf][i][j];
            }

            means[i][j] = sum_x / (VIO_Real) n_surfaces;
            variance[i][j] = (sum_xx - sum_x * sum_x / (VIO_Real) n_surfaces) /
                             (VIO_Real) (n_surfaces-1);
        }
        else
        {
            means[i][j] = 0.0;
            variance[i][j] = 0.0;
        }
    }

    for_less( i, 0, ni )
    for_less( j, 0, nj )
    {
        if( samples_valid[i][j] )
        {
            s = sqrt( variance[i][j] );

            se = s / sqrt( (VIO_Real) n_surfaces );

            if( se == 0.0 )
                t_stat[i][j] = 0.0;
            else
                t_stat[i][j] = means[i][j] / se;
        }
        else
            t_stat[i][j] = 0.0;

        if( i == 0 && j == 0 || t_stat[i][j] < min_t )
            min_t = t_stat[i][j];
        if( i == 0 && j == 0 || t_stat[i][j] > max_t )
            max_t = t_stat[i][j];
    }

    if( !outputting_flag )
    {
        close_file( file );
        return( 0 );
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

    sx = ((VIO_Real) Point_x(max_range) - (VIO_Real) Point_x(min_range)) / 1.0;
    sy = ((VIO_Real) Point_y(max_range) - (VIO_Real) Point_y(min_range)) /
         (VIO_Real) (ni-1);
    sz = ((VIO_Real) Point_z(max_range) - (VIO_Real) Point_z(min_range)) /
         (VIO_Real) (nj-1);

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
