#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    char   executable[] )
{
    static  char  usage_str[] = "\n\
Usage: %s  volume.mnc  input.tag\n\
\n\
     Computes the statistics for the volume intensity in the region of the\n\
     tags.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *tag_filename;
    Real                 x_min, x_max, mean, median, std_dev, *samples;
    Real                 voxel[N_DIMENSIONS];
    int                  int_voxel[N_DIMENSIONS];
    Volume               volume;
    int                  n_samples, i, n_volumes, n_tags;
    Real                 **tags;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &tag_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_tag_file( tag_filename, &n_volumes, &n_tags,
                        &tags, NULL, NULL, NULL, NULL, NULL ) != OK )
        return( 1 );

    if( n_tags == 0 )
        return( 0 );

    ALLOC( samples, n_tags );
    n_samples = 0;

    for_less( i, 0, n_tags )
    {
        convert_world_to_voxel( volume, tags[i][X], tags[i][Y], tags[i][Z],
                                voxel );
        if( voxel_is_within_volume( volume, voxel ) )
        {
            convert_real_to_int_voxel( 3, voxel, int_voxel );

            samples[n_samples] = get_volume_real_value( volume,
                                                        int_voxel[X],
                                                        int_voxel[Y],
                                                        int_voxel[Z], 0, 0 );
            ++n_samples;
        }
    }

    delete_volume( volume );

    free_tag_points( n_volumes, n_tags, tags, NULL, NULL, NULL, NULL, NULL );

    if( n_samples > 0 )
    {
        compute_statistics( n_samples, samples, &x_min, &x_max,
                            &mean, &std_dev, &median );

        print( "Min    : %g\n", x_min );
        print( "Max    : %g\n", x_max );
        print( "Mean   : %g\n", mean );
        print( "Median : %g\n", median );
        print( "Std Dev: %g\n", std_dev );
    }

    return( 0 );
}
