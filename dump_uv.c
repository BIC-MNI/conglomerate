#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int  argc,
    char *argv[] )
{
    FILE           *file;
    VIO_STR         input_filename, output_filename;
    VIO_File_formats   format;
    object_struct  **object_list;
    VIO_Point          centre;
    VIO_Real           u, v;
    int            n_objects, point;
    polygons_struct *polygons, unit_sphere;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s input.obj output.txt\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK )
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

    /*--- create a unit sphere with same number of triangles as surface */

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               polygons->n_items, &unit_sphere );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != VIO_OK )
        return( 1 );

    for_less( point, 0, polygons->n_points )
    {
        map_sphere_to_uv( RPoint_x(unit_sphere.points[point]),
                          RPoint_y(unit_sphere.points[point]),
                          RPoint_z(unit_sphere.points[point]), &u, &v );

        (void) output_real( file, u );
        (void) output_real( file, v );
        (void) output_newline( file );
    }

    (void) close_file( file );

    return( 0 );
}
