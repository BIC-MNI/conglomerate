#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename;
    char                 *output_filename, *dummy;
    Real                 min_value, max_value, value_to_set, value;
    int                  x, y, z, sizes[MAX_DIMENSIONS], n_changed;
    progress_struct      progress;
    Volume               volume;
    BOOLEAN              correct_label_range, set_value_specified;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &min_value ) ||
        !get_real_argument( 0.0, &max_value ) )
    {
        print( "Usage: %s  input.mnc  output.mnc  min_value  max_value set_value\n",
               argv[0] );
        print( "    [correct_label_range_flag]\n" );
        return( 1 );
    }

    set_value_specified = get_real_argument( 0.0, &value_to_set );
    correct_label_range = get_string_argument( "", &dummy );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( correct_label_range )
        set_label_volume_real_range( volume );

    if( !set_value_specified )
        value_to_set = get_volume_voxel_min( volume );

    get_volume_sizes( volume, sizes );

    n_changed = 0;

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Masking Volume" );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VALUE_3D( value, volume, x, y, z );
                if( min_value <= value && value <= max_value )
                {
                    SET_VOXEL_3D( volume, x, y, z, value_to_set );
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
                                   "Thresholded", NULL );

    return( 0 );
}
