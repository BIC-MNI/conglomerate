#include  <def_mni.h>

main(
   int   argc,
   char  *argv[] )
{
    VIO_Status   status;
    char     *input_filename, *output_filename;
    static   Colour  colour_table[] = { RED, GREEN, BLUE };
    FILE     *file;
    VIO_Volume   volume;
    static   VIO_STR  dim_names[] = { MIxspace, MIyspace, MIzspace };

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Arguments.\n" );
        return( 1 );
    }

    status = input_volume( input_filename, dim_names, &volume );

    alloc_auxiliary_data( volume );

    set_all_volume_auxiliary_data( volume, ACTIVE_BIT | 1 );

    if( status == VIO_OK )
        status = open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file );

    if( status == VIO_OK )
        status = output_labels_as_landmarks( file, volume, 1.0, 1,
                                             colour_table );

    if( status == VIO_OK )
        status = close_file( file );

    return( status != VIO_OK );
}
