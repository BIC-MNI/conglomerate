#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int  argc,
    char *argv[] )
{
    FILE           *file;
    STRING         input_filename, output_filename;
    File_formats   format;
    object_struct  **object_list;
    int            n_objects, point, poly, vertex, size;
    polygons_struct *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s input.obj output.txt\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
    {
        print( "Couldn't read %s.\n", input_filename );
        return( 1 );
    }

    if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
    {
        print( "File must contain exactly 1 polygons object.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    (void) output_int( file, polygons->n_points );
    (void) output_int( file, polygons->n_items );
    (void) output_newline( file );
    (void) output_newline( file );

    for_less( point, 0, polygons->n_points )
    {
        (void) output_real( file, RPoint_x(polygons->points[point]) );
        (void) output_real( file, RPoint_y(polygons->points[point]) );
        (void) output_real( file, RPoint_z(polygons->points[point]) );
        (void) output_newline( file );
    }

    (void) output_newline( file );

    for_less( point, 0, polygons->n_points )
    {
        (void) output_real( file, RPoint_x(polygons->normals[point]) );
        (void) output_real( file, RPoint_y(polygons->normals[point]) );
        (void) output_real( file, RPoint_z(polygons->normals[point]) );
        (void) output_newline( file );
    }

    (void) output_newline( file );

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        (void) output_int( file, size );
        (void) output_newline( file );
        for_less( vertex, 0, size )
        {
            point = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                                  vertex)];
            (void) output_int( file, point );
        }
        (void) output_newline( file );
    }

    (void) output_newline( file );

    (void) close_file( file );

    return( 0 );
}
