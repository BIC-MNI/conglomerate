#include  <def_mni.h>

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
    char                 *input_filename;
    char                 *output_filename;
    Volume               volume;
    Minc_file            minc_file;
    static String        dim_names[] = { MIxspace, MIyspace, MIzspace };

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    /* read the input volume */

    if( input_volume( input_filename, dim_names, FALSE, &volume ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );
    get_volume_range( volume, &min_value, &max_value );

    new_max = max_value - min_value;
    new_min = -new_max;

    for_less( x, 0, sizes[X] / 2 )
    {
        right_x = sizes[X] - 1 - x;
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VOXEL_3D( left_value, volume, x, y, z );
                left_value = CONVERT_VOXEL_TO_VALUE( volume, left_value );
                GET_VOXEL_3D( right_value, volume, right_x, y, z );
                right_value = CONVERT_VOXEL_TO_VALUE( volume, right_value );

                value = 255.0 * (left_value - right_value - new_min) /
                                (new_max - new_min);
                SET_VOXEL_3D( volume, x, y, z, value );

                value = 255.0 * (right_value - left_value - new_min) /
                                (new_max - new_min);
                SET_VOXEL_3D( volume, right_x, y, z, value );
            }
        }
    }

    minc_file = initialize_minc_output( output_filename, 3, dim_names,
                                   sizes,
                                   volume->nc_data_type,
                                   volume->signed_flag,
                                   new_min, new_max,
                                   &volume->voxel_to_world_transform );

    /* --- output the volume */

    status = output_minc_volume( minc_file, volume );

    if( status == OK )
        status = close_minc_output( minc_file );

    if( status != OK )
        print( "Unsuccessful.\n" );

    return( status != OK );
}
