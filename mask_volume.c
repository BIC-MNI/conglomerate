#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.mnc  mask_volume.mnc  output.mnc\n\
         [min] [max] [value_to_set]\n\
\n\
\n\
     Changes some voxels in the input volume to the value_to_set, which\n\
     defaults to the volume minimum.  A voxel is changed if the corresponding\n\
     voxel in the mask_volume has a value between min and max, which\n\
     defaults to values greater than zero.  The resulting volume is placed\n\
     in output.mnc.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename, mask_volume_filename;
    STRING               output_filename;
    Real                 mask_value, min_mask, max_mask;
    Real                 value_to_set;
    int                  x, y, z, sizes[MAX_DIMENSIONS], n_changed;
    progress_struct      progress;
    Volume               volume, mask_volume;
    BOOLEAN              value_specified, same_grid;
    minc_input_options   options;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &mask_volume_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.5, &min_mask );
    (void) get_real_argument( 1.0e30, &max_mask );
    value_specified = get_real_argument( 0.0, &value_to_set );


    set_default_minc_input_options( &options );
    set_minc_input_vector_to_colour_flag( &options, TRUE );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, &options ) != OK )
        return( 1 );

    if( equal_strings( volume_filename, mask_volume_filename ) )
    {
        mask_volume = volume;
    }
    else
    {
        if( input_volume_header_only( mask_volume_filename, 3,
                           XYZ_dimension_names, &mask_volume, &options) != OK )
            return( 1 );

        same_grid = volumes_are_same_grid( volume, mask_volume );

        delete_volume( mask_volume );

        if( same_grid )
        {
            if( input_volume( mask_volume_filename, 3, XYZ_dimension_names,
                              NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                              TRUE, &mask_volume, &options ) != OK )
                return( 1 );
        }
        else
        {
            mask_volume = create_label_volume( volume, NC_UNSPECIFIED );

            set_all_volume_label_data( mask_volume, 0 );

            if( load_label_volume( mask_volume_filename, mask_volume ) != OK )
                return( 1 );
        }
    }

    get_volume_sizes( volume, sizes );

    if( !value_specified )
        value_to_set = get_volume_real_min( volume );

    n_changed = 0;

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Masking Volume" );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                mask_value = get_volume_real_value( mask_volume, x, y, z, 0, 0);
                if( min_mask <= mask_value && mask_value <= max_mask )
                {
                    set_volume_real_value( volume, x, y, z, 0, 0, value_to_set);
                    ++n_changed;
                }
            }

            update_progress_report( &progress, x * sizes[Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    print( "Masked %d voxels\n", n_changed );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, volume_filename,
                                   "Masked", NULL );

    return( 0 );
}
