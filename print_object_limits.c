#include  <bicpl.h>
#include  <volume_io/internal_volume_io.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Point          *points, min_range, max_range, min_point, max_point;
    STRING         input_filename;
    int            obj, n_objects, n_points;
    File_formats   format;
    object_struct  **object_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list ) != OK )
        return( 1 );
    
    for_less( obj, 0, n_objects )
    {
        n_points = get_object_points( object_list[obj], &points );

        get_range_points( n_points, points, &min_point, &max_point );

        if( obj == 0 )
        {
            min_range = min_point;
            max_range = max_point;
        }
        else
        {
            expand_min_and_max_points( &min_range, &max_range,
                                       &min_point, &max_point );
        }
    }

    print( "X: %g %g\n", RPoint_x(min_range), RPoint_x(max_range) );
    print( "Y: %g %g\n", RPoint_y(min_range), RPoint_y(max_range) );
    print( "Z: %g %g\n", RPoint_z(min_range), RPoint_z(max_range) );

    delete_object_list( n_objects, object_list );

    return( 0 );
}
