#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input_lines.obj  output_lines.obj [n_points_per_segment]\n\
\n\
     Creates a cubic spline through the input line points.\n\
     The n_points_per_segment indicates the number of line points used\n\
     to approximate each spline segment, defaulting to 8.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               src_filename, dest_filename;
    File_formats         format;
    int                  n_points_per_segment;
    int                  i;
    int                  n_objects;
    object_struct        **objects;
    lines_struct         *lines, new_lines;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &dest_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 8, &n_points_per_segment );

    if( input_graphics_file( src_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type( objects[i] ) == LINES )
        {
            lines = get_lines_ptr(objects[i]);
            create_line_spline( lines, n_points_per_segment, &new_lines );
            delete_lines( lines );
            *lines = new_lines;
        }
    }

    if( output_graphics_file( dest_filename, format, n_objects, objects ) != OK)
        return( 1 );

    delete_object_list( n_objects, objects );

    return( 0 );
}
