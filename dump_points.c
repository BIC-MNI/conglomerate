#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int  argc,
    char *argv[] )
{
    FILE           *file;
    STRING         input_filename, output_filename, ascii_binary;
    File_formats   format;
    object_struct  **object_list;
    int            n_objects, n_points, i, obj;
    Point          *points;
    BOOLEAN        ascii_binary_present;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s input.obj output.pt [ascii|binary]\n" );
        return( 1 );
    }

    ascii_binary_present = get_string_argument( NULL, &ascii_binary );
    if( ascii_binary_present && ascii_binary[0] != 'a' &&
        ascii_binary[0] != 'b' )
    {
        print_error( "Usage: %s input.obj output.pt [ascii|binary]\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
    {
        print( "Couldn't read %s.\n", input_filename );
        return( 1 );
    }

    if( ascii_binary_present && ascii_binary[0] == 'a' )
        format = ASCII_FORMAT;
    else if( ascii_binary_present && ascii_binary[0] == 'b' )
        format = BINARY_FORMAT;

    if( open_file( output_filename, WRITE_FILE, format, &file ) != OK )
        return( 1 );

    for_less( obj, 0, n_objects )
    {
        n_points = get_object_points( object_list[obj], &points );

        for_less( i, 0, n_points )
        {
            if( io_point( file, WRITE_FILE, format, &points[i] ) != OK )
                return( 1 );

            if( format == ASCII_FORMAT && output_newline( file ) != OK )
                return( 1 );
        }
    }

    (void) close_file( file );

    return( 0 );
}
