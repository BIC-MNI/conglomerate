#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int  argc,
    char *argv[] )
{
    FILE           *file;
    STRING         input_filename, output_filename;
    File_formats   format;
    object_struct  **object_list;
    int            n_objects, n_points, i;
    Point          *points;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s input.obj output.txt\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
    {
        print( "Couldn't read %s.\n", input_filename );
        return( 1 );
    }

    if( n_objects != 1 )
    {
        print( "File must contain exactly 1 object.\n" );
        return( 1 );
    }

    n_points = get_object_points( object_list[0], &points );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( i, 0, n_points )
    {
        if( output_real( file, Point_x(points[i]) ) != OK ||
            output_real( file, Point_y(points[i]) ) != OK ||
            output_real( file, Point_z(points[i]) ) != OK ||
            output_newline( file ) != OK )
            return( 1 );
    }

    (void) close_file( file );

    return( 0 );
}
