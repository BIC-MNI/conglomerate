#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input1_filename, input2_filename, output_filename;
    int              i, n_objects;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons1, *polygons2;
    Point            point;
    Real             dist;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
          "Usage: %s  input1.obj  input2.obj output.txt\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input1_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input1_filename );
        return( 1 );
    }

    polygons1 = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( input2_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input2_filename );
        return( 1 );
    }

    polygons2 = get_polygons_ptr( object_list[0] );

    create_polygons_bintree( polygons2,
                             polygons2->n_items * BINTREE_FACTOR );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( i, 0, polygons1->n_points )
    {
        (void) find_closest_polygon_point( &polygons1->points[i], polygons2,
                                           &point );
        dist = distance_between_points( &polygons1->points[i], &point );

        if( output_real( file, dist ) != OK ||
            output_newline( file ) != OK )
            return( 1 );
    }

    (void) close_file( file );

    return( 0 );
}
