#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    char   executable[] )
{
    static  char  usage_str[] = "\n\
Usage: %s  volume.mnc  input.tag|input.mnc\n\
\n\
     Computes the statistics for the volume intensity in the region of the\n\
     tags or mask volume.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *label_filename;
    Real                 x_min, x_max, mean, median, std_dev, *samples, value;
    Volume               volume, label_volume;
    FILE                 *file;
    int                  n_samples, v[MAX_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &label_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_UNSPECIFIED );

    if( filename_extension_matches( label_filename,
                                    get_default_tag_file_suffix() ) )
    {
        if( open_file( label_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );

        if( input_tags_as_labels( file, volume, label_volume ) != OK )
            return( 1 );

        (void) close_file( file );
    }
    else
    {
        if( input_volume( label_filename, 3, XYZ_dimension_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          TRUE, &label_volume,
                          (minc_input_options *) NULL ) != OK )
            return( 1 );

        if( !volumes_are_same_grid( volume, label_volume ) )
        {
            print_error( "The label volume must match the intensity volume.\n");
            return( 1 );
        }
    }

    n_samples = 0;

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        if( get_volume_label_data( label_volume, v ) != 0 )
        {
            value = get_volume_real_value( volume, v[0], v[1], v[2], v[3],v[4]);

            ADD_ELEMENT_TO_ARRAY( samples, n_samples, value, 100000 );
        }

    END_ALL_VOXELS

    delete_volume( volume );

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
