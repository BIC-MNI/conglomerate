#include  <mni.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *src_filename, *dest_filename;
    File_formats         format;
    Real                 line_thickness;
    int                  i;
    int                  n_objects;
    object_struct        **objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &dest_filename ) )
    {
        print( "Usage: %s  src_polygons  dest_lines\n", argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 1.0, &line_thickness );

    if( input_graphics_file( src_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type( objects[i] ) == LINES )
            get_lines_ptr(objects[i])->line_thickness = (float) line_thickness;
    }

    if( output_graphics_file( dest_filename, format, n_objects, objects ) != OK)
        return( 1 );

    delete_object_list( n_objects, objects );

    return( 0 );
}
