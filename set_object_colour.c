#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  output.obj colour_name\n\
\n\
     Copies the input objects to the output objects, with the colour set\n\
     accordingly.\n\n";

    print_error( usage_str, executable );
}


int  main(
    int   argc,
    char  *argv[] )
{
    STRING               src_filename, dest_filename, colour_name;
    File_formats         format;
    Real                 line_thickness;
    int                  i;
    Colour               colour;
    int                  n_objects;
    object_struct        **objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &dest_filename ) ||
        !get_string_argument( "", &colour_name ) ||
        !lookup_colour( colour_name, &colour ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 1.0, &line_thickness );

    if( input_graphics_file( src_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
        set_object_colour( objects[i], colour );

    if( output_graphics_file( dest_filename, format, n_objects, objects ) != OK)
        return( 1 );

    delete_object_list( n_objects, objects );

    return( 0 );
}
