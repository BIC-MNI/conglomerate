#include <internal_volume_io.h>
#include <bicpl.h>

#define  BINTREE_FACTOR  0.3

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj\n\
\n\
     Writes out the number of items in the object.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename;
    int              n_objects;
    File_formats     format;
    object_struct    **object_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "File must contain exactly 1 polygons struct.\n" );
        return( 1 );
    }

    print( "%d\n", get_polygons_ptr( object_list[0] )->n_items );

    return( 0 );
}
