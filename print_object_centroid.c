#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj\n\
\n\
     Prints the centroid of the object.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING              input_filename;
    int                 i, n_objects, n_points;
    File_formats        format;
    object_struct       **object_list;
    Point               *points, centroid;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( object_list[i], &points );
        get_points_centroid( n_points, points, &centroid );
        print( "%g %g %g\n", RPoint_x( centroid ),
                             RPoint_y( centroid ),
                             RPoint_z( centroid ) );
    }

    delete_object_list( n_objects, object_list );

    return( 0 );
}
