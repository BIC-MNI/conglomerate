#include  <def_mni.h>

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  volume1_filename volume2_filename xform_filename output_prefix\n", executable_name );
}

int  main(
    int    argc,
    char   *argv[] )
{
    FILE                 *file;
    char                 *volume1_filename, *volume2_filename;
    char                 *transform_filename, *output_prefix;
    volume_struct        volume1, volume2;
    volume_struct        resampled;
    volume_input_struct  volume_input;
    int                  i, j;
    double               mni_transform[3][4];
    Transform            transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume1_filename ) ||
        !get_string_argument( "", &volume2_filename ) ||
        !get_string_argument( "", &transform_filename ) ||
        !get_string_argument( "", &output_prefix ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    /* just get the volume header, without reading the data */

    if( start_volume_input( volume1_filename, FALSE, &volume1,
                            &volume_input ) != VIO_OK )
        return( 1 );

    delete_volume( &volume1 );
    delete_volume_input( &volume_input );

    /* read the volume to be resampled */

    print( "Reading %s\n", volume2_filename );

    if( input_volume( volume2_filename, &volume2 ) != VIO_OK )
        return( 1 );

    /* read the transform */

    if( open_file_with_default_suffix( transform_filename, "xfm", READ_FILE,
                                       ASCII_FORMAT, &file ) != VIO_OK )
        return( 1 );

    if( !input_transform( file, mni_transform ) )
        return( 1 );

    for_less( i, 0, 3 )
        for_less( j, 0, 4 )
            Transform_elem(transform,i,j) = mni_transform[i][j];

    for_less( j, 0, 3 )
        Transform_elem(transform,3,j) = 0.0;

    Transform_elem(transform,3,3) = 1.0;

    (void) close_file( file );

    /* now create the resampled volume */

    resampled = volume1;

    resampled.data_type = volume2.data_type;

    alloc_volume( &resampled );

    resample_volume( &volume2, &transform, &resampled );

    (void) output_volume( output_prefix, &resampled,
                          volume1.axis_index_from_file );

    return( 0 );
}
