#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    static  STRING  usage_str = "\n\
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
    STRING               volume_filename, label_filename, dummy;
    STRING               dump_filename;
    Real                 mean, median, std_dev, value;
    Real                 min_sample_value, max_sample_value;
    Volume               volume, label_volume;
    BOOLEAN              median_required, first_pass;
    Real                 separations[MAX_DIMENSIONS];
    Real                 min_world[MAX_DIMENSIONS], max_world[MAX_DIMENSIONS];
    Real                 min_voxel[MAX_DIMENSIONS], max_voxel[MAX_DIMENSIONS];
    Real                 min_xyz_voxel[MAX_DIMENSIONS];
    Real                 max_xyz_voxel[MAX_DIMENSIONS], voxel1, voxel2;
    Real                 real_v[MAX_DIMENSIONS], world[MAX_DIMENSIONS];
    Real                 min_value, max_value;
    Real                 median_min, median_max, median_error;
    FILE                 *file;
    BOOLEAN              dumping, labels_present, first, done;
    int                  c, n_samples, v[MAX_DIMENSIONS];
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
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_separations( volume, separations );
    get_volume_real_range( volume, &min_value, &max_value );

    labels_present = !equal_strings( label_filename, "none" );

    if( labels_present )
    {
        label_volume = create_label_volume( volume, NC_UNSPECIFIED );

        set_all_volume_label_data( label_volume, 0 );

        if( filename_extension_matches( label_filename,
                                        get_default_tag_file_suffix() ) )
        {
            if( open_file( label_filename, READ_FILE, ASCII_FORMAT, &file )!=OK)
                return( 1 );

            if( input_tags_as_labels( file, volume, label_volume ) != OK )
                return( 1 );

            (void) close_file( file );
        }
        else
        {
            if( load_label_volume( label_filename, label_volume ) != OK )
                return( 1 );
        }
    }

    if( dumping )
    {
        if( open_file( dump_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
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
                        if( output_real( file, value ) != OK ||
                            output_newline( file ) != OK )
                            return( 1 );
                    }

                    for_less( c, 0, N_DIMENSIONS )
                        real_v[c] = (Real) v[c];

                    convert_voxel_to_world( volume, real_v,
                                            &world[X], &world[Y], &world[Z]);

                    if( first )
                    {
                        first = FALSE;
                        for_less( c, 0, N_DIMENSIONS )
                        {
                            min_voxel[c] = real_v[c];
                            max_voxel[c] = real_v[c];
                            min_world[c] = world[c];
                            max_world[c] = world[c];
                        }
                    }
                    else
                    {
                        for_less( c, 0, N_DIMENSIONS )
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
            if( get_volume_data_type(volume) != FLOAT &&
                get_volume_data_type(volume) != DOUBLE )
            {
                median_min = convert_value_to_voxel( volume,
                                                     median - median_error );
                median_max = convert_value_to_voxel( volume,
                                                     median + median_error );

                median_min = ROUND( median_min );
                median_max = ROUND( median_max );

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
        print( "Volume   : %g\n",
               n_samples * separations[X] * separations[Y] * separations[Z] );
        print( "Min      : %g\n", min_sample_value );
        print( "Max      : %g\n", max_sample_value );
        print( "Mean     : %g\n", mean );
        if( median_error == 0.0 )
            print( "Median   : %g\n", median );
        print( "Std Dev  : %g\n", std_dev );
        print( "Voxel Rng:" );
        for_less( c, 0, N_DIMENSIONS )
            print( " %g", min_xyz_voxel[c] );
        print( "\n" );
        print( "         :" );
        for_less( c, 0, N_DIMENSIONS )
            print( " %g", max_xyz_voxel[c] );
        print( "\n" );
        print( "World Rng:" );
        for_less( c, 0, N_DIMENSIONS )
            print( " %g", min_world[c] );
        print( "\n" );
        print( "         :" );
        for_less( c, 0, N_DIMENSIONS )
            print( " %g", max_world[c] );
        print( "\n" );
    }
    else
        print( "No samples found.\n" );

    return( 0 );
}
