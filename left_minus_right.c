#include  <mni.h>

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_filename  [input_filename2]  output_filename\n", executable_name );
}

int  main(
    int    argc,
    char   *argv[] )
{
    Status               status;
    int                  sizes1[MAX_DIMENSIONS], sizes2[MAX_DIMENSIONS];
    int                  x, y, z, right_x;
    Real                 left_value, right_value, min_value1, max_value1;
    Real                 min_value2, max_value2;
    Real                 new_min, new_max, value;
    char                 *input_filename1, *input_filename2;
    char                 *output_filename;
    Volume               volume1, volume2, new_volume;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename1 ) ||
        !get_string_argument( "", &input_filename2 ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( !get_string_argument( "", &output_filename ) )
    {
        output_filename = input_filename2;
        input_filename2 = (char *) NULL;
    }

    /* read the input volume */

    if( input_volume( input_filename1, 3, XYZ_dimension_names, NC_UNSPECIFIED, FALSE,
                    0.0, 0.0, TRUE, &volume1, (minc_input_options *) 0 ) != OK )
        return( 1 );

    if( input_filename2 != (char *) NULL )
    {
        if( input_volume( input_filename2, 3, XYZ_dimension_names, NC_UNSPECIFIED, FALSE,
                    0.0, 0.0, TRUE, &volume2, (minc_input_options *) 0 ) != OK )
        return( 1 );
    }
    else
        volume2 = volume1;

    get_volume_sizes( volume1, sizes1 );
    get_volume_sizes( volume2, sizes2 );

    if( sizes1[X] != sizes2[X] || sizes1[Y] != sizes2[Y] ||
        sizes1[Z] != sizes2[Z] )
    {
        print( "Sizes do not match.\n" );
        return( 1 );
    }

    get_volume_real_range( volume1, &min_value1, &max_value1 );
    get_volume_real_range( volume2, &min_value2, &max_value2 );

    new_max = MAX( max_value1 - min_value2, max_value2 - min_value1 );
    new_min = -new_max;

    new_volume = copy_volume_definition( volume1, NC_BYTE, FALSE, 0.0, 255.0 );

    set_volume_voxel_range( new_volume, 0.0, 255.0 );
    set_volume_real_range( new_volume, new_min, new_max );

    alloc_volume_data( new_volume );

    initialize_progress_report( &progress, FALSE,
                                (sizes1[X] / 2 +1) * sizes1[Y],
                                "Subtracting" );

    for_less( x, 0, sizes1[X] / 2 + 1 )
    {
        right_x = sizes1[X] + 1 - x;
        if( right_x >= sizes1[X] )
            right_x = x;
        for_less( y, 0, sizes1[Y] )
        {
            for_less( z, 0, sizes1[Z] )
            {
                GET_VOXEL_3D( left_value, volume1, x, y, z );
                left_value = convert_voxel_to_value( volume1, left_value );
                GET_VOXEL_3D( right_value, volume2, right_x, y, z );
                right_value = convert_voxel_to_value( volume2, right_value );

                value = convert_value_to_voxel( new_volume,
                                                left_value - right_value );
                SET_VOXEL_3D( new_volume, x, y, z, value );

                value = convert_value_to_voxel( new_volume,
                                                right_value - left_value );
                SET_VOXEL_3D( new_volume, right_x, y, z, value );
            }

            update_progress_report( &progress, x * sizes1[Y] + y + 1 );
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
