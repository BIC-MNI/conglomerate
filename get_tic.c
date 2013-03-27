#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    static  VIO_STR  usage_str = "\n\
Usage: %s filename.mnc  x y z v|w [radius1] [radius2] [radius3]\n\
\n\
     Computes the min, max, mean, standard deviation, and median for a set\n\
     of spherical regions in the specified volume.\n\
\n\
     If the fifth argument is 'v', then the previous three arguments are voxel\n\
     positions, in the order of x y z, not necessarily that of the file.\n\
     If the fifth argument is 'w', the previous three arguments are in \n\
     world space.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename, voxel_flag_string;
    VIO_STR               component_string;
    char                 buffer[VIO_EXTREMELY_LARGE_STRING_SIZE];
    VIO_Real                 mean, median, std_dev, value, pos[VIO_N_DIMENSIONS];
    VIO_Real                 min_sample_value, max_sample_value;
    VIO_Volume               volume;
    VIO_Real                 separations[VIO_MAX_DIMENSIONS];
    int                  min_voxel[VIO_MAX_DIMENSIONS], max_voxel[VIO_MAX_DIMENSIONS];
    int                  start_voxel[VIO_MAX_DIMENSIONS], end_voxel[VIO_MAX_DIMENSIONS];
    int                  n_dims, axis, voxel_index;
    VIO_STR               *dim_names;
    VIO_Real                 min_value, max_value, radius;
    VIO_Real                 voxel_radius[VIO_MAX_DIMENSIONS], diff, dist;
    VIO_Real                 voxel_centre[VIO_MAX_DIMENSIONS];
    VIO_Real                 min_pos, max_pos, median_error;
    VIO_BOOL              done, use_world_space;
    VIO_BOOL              is_spatial_dim[VIO_MAX_DIMENSIONS];
    int                  n_samples;
    int                  sizes[VIO_MAX_DIMENSIONS], voxel[VIO_MAX_DIMENSIONS];
    int                  dim, n_spatial_dims, spatial_dims[VIO_N_DIMENSIONS];
    statistics_struct    stats;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_real_argument( 0.0, &pos[0] ) ||
        !get_real_argument( 0.0, &pos[1] ) ||
        !get_real_argument( 0.0, &pos[2] ) ||
        !get_string_argument( NULL, &voxel_flag_string ) ||
        string_length( voxel_flag_string ) != 1 ||
        voxel_flag_string[0] != 'v' && voxel_flag_string[0] != 'w' &&
        voxel_flag_string[0] != 'V' && voxel_flag_string[0] != 'W' )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( volume_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    n_dims = get_volume_n_dimensions( volume );
    get_volume_sizes( volume, sizes );
    get_volume_separations( volume, separations );
    get_volume_real_range( volume, &min_value, &max_value );
    dim_names = get_volume_dimension_names( volume );

    for_less( dim, 0, VIO_N_DIMENSIONS )
        spatial_dims[dim] = -1;

    n_spatial_dims = 0;

    for_less( dim, 0, n_dims )
    {
        if( convert_dim_name_to_spatial_axis( dim_names[dim], &axis ) )
        {
            spatial_dims[axis] = dim;
            ++n_spatial_dims;
            start_voxel[dim] = 0;
            end_voxel[dim] = 0;
            is_spatial_dim[dim] = TRUE;
        }
        else
        {
            start_voxel[dim] = 0;
            end_voxel[dim] = sizes[dim]-1;
            is_spatial_dim[dim] = FALSE;
        }
    }

    for_less( dim, n_dims, VIO_MAX_DIMENSIONS )
    {
        start_voxel[dim] = 0;
        end_voxel[dim] = 0;
    }

    for_less( dim, n_spatial_dims, VIO_MAX_DIMENSIONS )
    {
        min_voxel[dim] = 0;
        max_voxel[dim] = 0;
    }

    if( n_spatial_dims != VIO_N_DIMENSIONS )
    {
        print_error( "Warning: VIO_Volume has %d spatial dimensions.\n",
                      n_spatial_dims );
        return( 1 );
    }

    use_world_space = (voxel_flag_string[0] == 'w' ||
                       voxel_flag_string[0] == 'W');

    if( use_world_space )
    {
        convert_world_to_voxel( volume, pos[0], pos[1], pos[2], voxel_centre );
    }
    else
    {
        for_less( dim, 0, n_dims )
            voxel_centre[dim] = 0.0;

        for_less( dim, 0, n_spatial_dims )
            voxel_centre[spatial_dims[dim]] = pos[dim] - 1.0;
    }

    print( "Radius    Comp       Min        Max       Median     Mean      Std Dev \n" );
    print( "------  --------  ---------  ---------  ---------  ---------  ---------\n" );

    while( get_real_argument( 0.0, &radius ) )
    {
        for_less( dim, 0, n_dims )
        {
            voxel_radius[dim] = 0.0;
            min_voxel[dim] = 0;
            max_voxel[dim] = 0;
        }

        for_less( dim, 0, n_spatial_dims )
        {
            voxel_index = spatial_dims[dim];

            voxel_radius[voxel_index] = radius /
                                       VIO_FABS(separations[voxel_index]);

            min_pos = voxel_centre[voxel_index] - voxel_radius[voxel_index];
            max_pos = voxel_centre[voxel_index] + voxel_radius[voxel_index];

            min_voxel[dim] = MAX( 0, VIO_ROUND(min_pos) );
            max_voxel[dim] = MIN( sizes[voxel_index]-1, VIO_ROUND(max_pos) );
        }

        for_inclusive( voxel[0], start_voxel[0], end_voxel[0] )
        for_inclusive( voxel[1], start_voxel[1], end_voxel[1] )
        for_inclusive( voxel[2], start_voxel[2], end_voxel[2] )
        for_inclusive( voxel[3], start_voxel[3], end_voxel[3] )
        for_inclusive( voxel[4], start_voxel[4], end_voxel[4] )
        {
            initialize_statistics( &stats, min_value, max_value );

            do
            {
                done = FALSE;
                for_less( dim, 0, n_spatial_dims )
                {
                    voxel[spatial_dims[dim]] = min_voxel[dim];
                    if( voxel[spatial_dims[dim]] > max_voxel[dim] )
                        done = TRUE;
                }

                while( !done )
                {
                    dist = 0.0;
                    for_less( dim, 0, n_spatial_dims )
                    {
                        voxel_index = spatial_dims[dim];

                        if( voxel_radius[voxel_index] > 0.0 )
                        {
                            diff = ((VIO_Real) voxel[voxel_index] -
                                           voxel_centre[voxel_index]) /
                                   voxel_radius[voxel_index];

                            dist += diff * diff;
                        }
                    }

                    if( dist <= 1.0 )
                    {
                        value = get_volume_real_value( volume, voxel[0],
                                  voxel[1], voxel[2], voxel[3], voxel[4] );

                        add_sample_to_statistics( &stats, value );
                    }

                    dim = n_spatial_dims - 1;

                    do
                    {
                        ++voxel[spatial_dims[dim]];
                        if( voxel[spatial_dims[dim]] <= max_voxel[dim] )
                            break;

                        voxel[spatial_dims[dim]] = min_voxel[dim];

                        --dim;
                    }
                    while( dim >= 0 );

                    if( dim < 0 )
                        done = TRUE;
                }

                get_statistics( &stats, &n_samples, &mean, &median,
                                &median_error,
                                &min_sample_value, &max_sample_value,
                                &std_dev );

                if( median_error > 0.0 )
                    restart_statistics_with_narrower_median_range( &stats );
            }
            while( median_error > 0.0 );

            terminate_statistics( &stats );

            component_string = create_string( NULL );

            if( n_dims != n_spatial_dims )
            {
                for_less( dim, 0, n_dims )
                {
                    if( !is_spatial_dim[dim] )
                    {
                        (void) sprintf( buffer, "[%d]", voxel[dim] );
                        concat_to_string( &component_string, buffer );
                    }
                }
            }

            if( n_samples < 1 )
                print( "%5g   %8s     n/a        n/a        n/a        n/a        n/a\n",
                        radius, component_string );
            else
            {
                print( "%5g   %8s  %9g  %9g  %9g  %9g",
                       radius, component_string,
                       min_sample_value, max_sample_value, median,
                       mean );

                if( n_samples == 1 )
                    print( "     n/a   \n" );
                else
                    print( "  %9g\n", std_dev );
            }
        }
    }

    return( 0 );
}
