#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE                 *file;
    STRING               src_filename, transform_filename, output_filename;
    int                  i, p, n_objects, n_points;
    Point                *points, trans;
    Real                 distance, x, y, z;
    General_transform    transform;
    File_formats         format;
    object_struct        **object_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &transform_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print_error( "Usage: %s src.obj xform.xfm output.txt\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( input_transform_file( transform_filename, &transform ) != OK )
        return( 1 );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( object_list[i], &points );

        for_less( p, 0, n_points )
        {
            general_transform_point( &transform,
                                     Point_x(points[p]),
                                     Point_y(points[p]),
                                     Point_z(points[p]), &x, &y, &z );

            fill_Point( trans, x, y, z );

            distance = distance_between_points( &points[p], &trans );

            (void) output_real( file, distance );
            (void) output_newline( file );
        }
    }

    (void) close_file( file );

    return( 0 );
}
