#include  <mni.h>

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_filename  output_filename\n", executable_name );
}

int  main(
    int    argc,
    char   *argv[] )
{
    Status               status;
    int                  sizes[3], x, y, z, right_x;
    Real                 left_value, right_value, min_value, max_value;
    Real                 new_min, new_max, value;
    Real                 separations[MAX_DIMENSIONS];
    char                 *input_filename;
    char                 *output_filename;
    Volume               volume, new_volume;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    /* read the input volume */

    if( input_volume( input_filename, 3, XYZ_dimension_names, NC_UNSPECIFIED, FALSE,
                    0.0, 0.0, TRUE, &volume, (minc_input_options *) 0 ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );
    get_volume_real_range( volume, &min_value, &max_value );
    get_volume_separations( volume, separations );

    new_max = max_value - min_value;
    new_min = -new_max;

    new_volume = copy_volume_definition( volume, NC_BYTE, FALSE, 0.0, 255.0 );

    set_volume_voxel_range( new_volume, 0.0, 255.0 );
    set_volume_real_range( new_volume, new_min, new_max );

    alloc_volume_data( new_volume );

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Subtracting" );

    for_less( x, 0, sizes[X] / 2 + 1 )
    {
        right_x = sizes[X] + 1 - x;
        if( right_x >= sizes[X] )
            right_x = x;
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VOXEL_3D( left_value, volume, x, y, z );
                left_value = CONVERT_VOXEL_TO_VALUE( volume, left_value );
                GET_VOXEL_3D( right_value, volume, right_x, y, z );
                right_value = CONVERT_VOXEL_TO_VALUE( volume, right_value );

                value = CONVERT_VALUE_TO_VOXEL( new_volume,
                                                left_value - right_value );
                SET_VOXEL_3D( new_volume, x, y, z, value );

                value = CONVERT_VALUE_TO_VOXEL( new_volume,
                                                right_value - left_value );
                SET_VOXEL_3D( new_volume, right_x, y, z, value );
            }

            update_progress_report( &progress, x * sizes[Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    status = output_volume( output_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            new_volume, "left-minus-right",
                            (minc_output_options *) NULL );

    if( status != OK )
        print( "Unsuccessful.\n" );

    return( status != OK );
}
