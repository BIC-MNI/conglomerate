#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE            *file;
    Status          status;
    char            *input_filename, *output_filename;
    int             n_triangles, n_objects;
    Point           centre;
    object_struct   *object, **object_list;
    polygons_struct *polygons;
    Real            x, y, z, rx, ry, rz;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        (void) fprintf( stderr,
             "Usage:  input_filename output_filename [n_triangles]\n" );
        return( 1 );
    }

    (void) get_int_argument( 128, &n_triangles );

    if( open_file( input_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    n_objects = 0;

    while( input_real( file, &x ) == OK &&
           input_real( file, &y ) == OK &&
           input_real( file, &z ) == OK &&
           input_real( file, &rx ) == OK &&
           input_real( file, &ry ) == OK &&
           input_real( file, &rz ) == OK )
    {
        object = create_object( POLYGONS );
        polygons = get_polygons_ptr(object);
        fill_Point( centre, x, y, z );
        create_tetrahedral_sphere( &centre, rx, ry, rz, n_triangles, polygons );
        compute_polygon_normals( polygons );

        ADD_ELEMENT_TO_ARRAY( object_list, n_objects, object, 10 );
    }

    status = output_graphics_file( output_filename, BINARY_FORMAT, n_objects,
                                   object_list );

    return( status != OK );
}
