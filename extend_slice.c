#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    char   executable[] )
{
    char  usage_str[] = "\n\
Usage: %s  input.mnc  mask_volume.mnc  output.mnc\n\
         [min] [max] [correct_label_range]\n\
\n\
\n\
     Changes all voxels in the input volume to the min value, if they\n\
     correspond to a value in the mask_volume between min and max, which\n\
     default to 0.  The resulting volume is placed in output.mnc.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *mask_volume_filename;
    char                 *output_filename, *dummy;
    Real                 mask_value, set_voxel, min_mask, max_mask;
    int                  x, y, z, sizes[MAX_DIMENSIONS], n_changed;
    progress_struct      progress;
    Volume               volume, mask_volume;
    BOOLEAN              correct_label_range;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &mask_volume_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_mask );
    (void) get_real_argument( 0.0, &max_mask );

    correct_label_range = get_string_argument( "", &dummy );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( correct_label_range )
        set_label_volume_real_range( volume );

    mask_volume = create_label_volume( volume, NC_UNSPECIFIED );

    set_all_volume_label_data( mask_volume, 0 );

    if( load_label_volume( mask_volume_filename, mask_volume ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );
    set_voxel = get_volume_voxel_min( volume );

    n_changed = 0;

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Masking Volume" );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VALUE_3D( mask_value, mask_volume, x, y, z );
                if( min_mask <= mask_value && mask_value <= max_mask )
                {
                    SET_VOXEL_3D( volume, x, y, z, set_voxel );
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
