#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    static  VIO_STR  usage_str = "\n\
Usage: %s  volume.mnc  input.tag|input.mnc|none  [dump_file|none] [median]\n\
\n\
     Computes the statistics for the volume intensity of the volume.  If\n\
     an input tag or label file is specified, then only those voxels in\n\
     in the region of the tags or mask volume or considered.   If a\n\
     third argument (other than the word none) is specified, all the\n\
     intensities are placed in the file.  If a fourth argument is specified,\n\
     then the median is also computed, which increases the time required.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename, label_filename, dummy;
    VIO_STR               dump_filename;
    VIO_Real                 mean, median, std_dev, value;
    VIO_Real                 min_sample_value, max_sample_value;
    VIO_Volume               volume, label_volume;
    VIO_BOOL              median_required, first_pass;
    VIO_Real                 separations[VIO_MAX_DIMENSIONS];
    VIO_Real                 min_world[VIO_MAX_DIMENSIONS], max_world[VIO_MAX_DIMENSIONS];
    VIO_Real                 min_voxel[VIO_MAX_DIMENSIONS], max_voxel[VIO_MAX_DIMENSIONS];
    VIO_Real                 min_xyz_voxel[VIO_MAX_DIMENSIONS];
    VIO_Real                 max_xyz_voxel[VIO_MAX_DIMENSIONS];
    VIO_Real                 real_v[VIO_MAX_DIMENSIONS], world[VIO_MAX_DIMENSIONS];
    VIO_Real                 min_value, max_value;
    VIO_Real                 median_min, median_max, median_error;
    FILE                 *file;
    VIO_BOOL              dumping, labels_present, first, done;
    int                  c, n_samples, v[VIO_MAX_DIMENSIONS];
    statistics_struct    stats;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &label_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    dumping = get_string_argument( NULL, &dump_filename ) &&
              !equal_strings( dump_filename, "none" );
    median_required = get_string_argument( NULL, &dummy );

    set_cache_block_sizes_hint( SLICE_ACCESS );

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    get_volume_separations( volume, separations );
    get_volume_real_range( volume, &min_value, &max_value );

    labels_present = !equal_strings( label_filename, "none" );

    if( labels_present )
    {
        if( equal_strings( label_filename, volume_filename ) )
            label_volume = volume;
        else
        {
            label_volume = create_label_volume( volume, NC_UNSPECIFIED );

            set_all_volume_label_data( label_volume, 0 );

            if( filename_extension_matches( label_filename,
                                            get_default_tag_file_suffix() ) )
            {
                if( open_file( label_filename, READ_FILE, ASCII_FORMAT, &file )!=VIO_OK)
                    return( 1 );

                if( input_tags_as_labels( file, volume, label_volume ) != VIO_OK )
                    return( 1 );

                (void) close_file( file );
            }
            else
            {
                if( load_label_volume( label_filename, label_volume ) != VIO_OK )
                    return( 1 );
            }
        }
    }

    if( dumping )
    {
        if( open_file( dump_filename, WRITE_FILE, ASCII_FORMAT, &file ) != VIO_OK )
            return( 1 );
    }

    first = TRUE;
    first_pass = TRUE;

    initialize_statistics( &stats, min_value, max_value );

    done = FALSE;

    while( !done )
    {
        BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

            if( !labels_present || get_volume_label_data( label_volume, v )!= 0)
            {
                value = get_volume_real_value( volume, v[0], v[1], v[2], v[3],
                                               v[4]);

                add_sample_to_statistics( &stats, value );

                if( first_pass )
                {
                    if( dumping )
                    {
                        if( output_real( file, value ) != VIO_OK ||
                            output_newline( file ) != VIO_OK )
                            return( 1 );
                    }

                    for_less( c, 0, VIO_N_DIMENSIONS )
                        real_v[c] = (VIO_Real) v[c];

                    convert_voxel_to_world( volume, real_v,
                                            &world[VIO_X], &world[VIO_Y], &world[VIO_Z]);

                    if( first )
                    {
                        first = FALSE;
                        for_less( c, 0, VIO_N_DIMENSIONS )
                        {
                            min_voxel[c] = real_v[c];
                            max_voxel[c] = real_v[c];
                            min_world[c] = world[c];
                            max_world[c] = world[c];
                        }
                    }
                    else
                    {
                        for_less( c, 0, VIO_N_DIMENSIONS )
                        {
                            if( real_v[c] < min_voxel[c] )
                                min_voxel[c] = real_v[c];
                            else if( real_v[c] > max_voxel[c] )
                                max_voxel[c] = real_v[c];

                            if( world[c] < min_world[c] )
                                min_world[c] = world[c];
                            else if( world[c] > max_world[c] )
                                max_world[c] = world[c];
                        }
                    }
                }
            }

        END_ALL_VOXELS

        first_pass = FALSE;

        get_statistics( &stats, &n_samples, &mean, &median, &median_error,
                        &min_sample_value, &max_sample_value, &std_dev );

        done = TRUE;

        if( median_required && median_error > 0.0 )
        {
            done = FALSE;
            if( get_volume_data_type(volume) != VIO_FLOAT &&
                get_volume_data_type(volume) != VIO_DOUBLE )
            {
                median_min = convert_value_to_voxel( volume,
                                                     median - median_error );
                median_max = convert_value_to_voxel( volume,
                                                     median + median_error );

                median_min = (VIO_Real) VIO_ROUND( median_min );
                median_max = (VIO_Real) VIO_ROUND( median_max );

                if( median_min == median_max )
                {
                    median = convert_voxel_to_value( volume, median_min );
                    median_error = 0.0;
                    done = TRUE;
                }
            }
        }

        if( !done )
            restart_statistics_with_narrower_median_range( &stats );
    }

    if( dumping )
        (void) close_file( file );

    terminate_statistics( &stats );

    delete_volume( volume );

    if( labels_present )
        delete_volume( label_volume );

    if( n_samples > 0 )
    {
        reorder_voxel_to_xyz( volume, min_voxel, min_xyz_voxel );
        reorder_voxel_to_xyz( volume, max_voxel, max_xyz_voxel );

        print( "N Voxels : %d\n", n_samples );
        print( "VIO_Volume   : %g\n",
                      (VIO_Real) n_samples * separations[VIO_X] * separations[VIO_Y] *
                                         separations[VIO_Z] );
        print( "Min      : %g\n", min_sample_value );
        print( "Max      : %g\n", max_sample_value );
        print( "Mean     : %g\n", mean );
        if( median_error == 0.0 )
            print( "Median   : %g\n", median );
        print( "Std Dev  : %g\n", std_dev );
        print( "Voxel Rng:" );
        for_less( c, 0, VIO_N_DIMENSIONS )
            print( " %g", min_xyz_voxel[c] );
        print( "\n" );
        print( "         :" );
        for_less( c, 0, VIO_N_DIMENSIONS )
            print( " %g", max_xyz_voxel[c] );
        print( "\n" );
        print( "World Rng:" );
        for_less( c, 0, VIO_N_DIMENSIONS )
            print( " %g", min_world[c] );
        print( "\n" );
        print( "         :" );
        for_less( c, 0, VIO_N_DIMENSIONS )
            print( " %g", max_world[c] );
        print( "\n" );
    }
    else
        print( "No samples found.\n" );

    return( 0 );
}
