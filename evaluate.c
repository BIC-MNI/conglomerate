#include  <bicpl.h>
#include  <internal_volume_io.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE                 *file;
    Volume               volume;
    STRING               input_filename, output_filename;
    STRING               volume_filename;
    int                  i, n_objects, p, n_points, degree;
    Real                 value;
    Point                *points;
    File_formats         format;
    object_struct        **object_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print_error( "Usage: %s  volume.mnc  input.obj  output.txt\n", argv[0]);
        return( 1 );
    }

    (void) get_int_argument( 0, &degree );

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( object_list[i], &points );

        for_less( p, 0, n_points )
        {
            evaluate_volume_in_world( volume,
                                      (Real) Point_x(points[p]),
                                      (Real) Point_y(points[p]),
                                      (Real)  Point_z(points[p]),
                                      degree, FALSE, 0.0, &value,
                                      NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, NULL, NULL );

            (void) output_real( file, value );
            (void) output_newline( file );
        }
    }

    (void) close_file( file );

    delete_object_list( n_objects, object_list );

    return( 0 );
}
