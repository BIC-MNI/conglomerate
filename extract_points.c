#include  <def_mni.h>
#include  <def_module.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status         status;
    FILE           *file;
    char           *input_filename, *output_filename;
    int            i, p, n_objects, n_points;
    Point          *points;
    File_formats   format;
    object_struct  **object_list;

    if( argc == 1 )
    {
        (void) fprintf( stderr, "Must have an input and output argument.\n" );
        return( 1 );
    }

    input_filename = argv[1];
    output_filename = argv[2];

    status = input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list );

    if( status == OK )
        status = open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file );

    if( status == OK )
    {
        for_less( i, 0, n_objects )
        {
            if( status == OK )
            {
                n_points = get_object_points( object_list[i], &points );
                (void) fprintf( file, "%d\n", n_points );
                for_less( p, 0, n_points )
                    (void) fprintf( file, "%g %g %g\n",
                                    Point_x(points[p]),
                                    Point_y(points[p]),
                                    Point_z(points[p]) );
            }
        }
    }

    if( status == OK )
        status = close_file( file );

    if( status == OK )
        delete_object_list( n_objects, object_list );

    return( status != OK );
}
