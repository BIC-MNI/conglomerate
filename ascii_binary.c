#include  <mni.h>
#include  <module.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status         status;
    char           *input_filename, *output_filename;
    int            n_objects;
    File_formats   format;
    object_struct  **object_list;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );

    status = input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list );

    if( format == BINARY_FORMAT )
        format = ASCII_FORMAT;
    else
        format = BINARY_FORMAT;

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       n_objects, object_list );

    if( status == OK )
        delete_object_list( n_objects, object_list );

    return( status != OK );
}
