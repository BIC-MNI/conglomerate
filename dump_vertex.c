#include  <bicpl.h>
#include  <volume_io.h>

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_Point          *points;
    VIO_STR         input_filename;
    int            vertex, n_objects, n_points;
    File_formats   format;
    object_struct  **object_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK )
        return( 1 );

    n_points = get_object_points( object_list[0], &points );

    while( get_int_argument( 0, &vertex ) )
    {
        if( vertex >= 0 && vertex < n_points )
            print( "%g %g %g\n", RPoint_x(points[vertex]),
                                 RPoint_y(points[vertex]),
                                 RPoint_z(points[vertex]) );
    }

    return( 0 );
}
