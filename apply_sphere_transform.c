#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  surface.obj  u.txt v.txt  output.obj\n\
\n\
     .\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING              surface_filename, output_filename;
    STRING              u_filename, v_filename;
    File_formats        format;
    int                 n_objects, p;
    Real                u, v, u_off, v_off, x, y, z;
    FILE                *file1, *file2;
    Point               centre, unit_point, *new_points;
    polygons_struct     *polygons, unit_sphere;
    object_struct       **objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &u_filename ) ||
        !get_string_argument( NULL, &v_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( surface_filename,
                             &format, &n_objects, &objects ) != OK ||
        n_objects != 1 || get_object_type(objects[0]) != POLYGONS )
    {
        print_error( "Surface file must contain one polygons struct\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( objects[0] );

    create_polygons_bintree( polygons,
                             ROUND( (Real) polygons->n_items * 0.2 ) );

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0, polygons->n_items,
                               &unit_sphere );

    if( open_file( u_filename, READ_FILE, ASCII_FORMAT, &file1 ) != OK ||
        open_file( v_filename, READ_FILE, ASCII_FORMAT, &file2 ) != OK )
        return( 1 );

    ALLOC( new_points, polygons->n_points );

    for_less( p, 0, polygons->n_points )
    {
        if( input_real( file1, &u_off ) != OK ||
            input_real( file2, &v_off ) != OK )
        {
            print_error( "Error reading u and v.\n" );
            return( 1 );
        }

        map_point_to_unit_sphere( polygons, &polygons->points[p],
                                  &unit_sphere, &unit_point );
        map_sphere_to_uv( RPoint_x(unit_point), RPoint_y(unit_point),
                          RPoint_z(unit_point), &u, &v );

        u += u_off;
        v += v_off;
        while( u < 0.0 )
            u += 1.0;
        while( u >= 1.0 )
            u -= 1.0;

        if( v < 0.0 )
            v = 0.0;
        if( v > 1.0 )
            v = 1.0;

        map_uv_to_sphere( u, v, &x, &y, &z );
        fill_Point( unit_point, x, y, z );
        map_unit_sphere_to_point( &unit_sphere, &unit_point,
                                  polygons, &new_points[p] );
    }

    for_less( p, 0, polygons->n_points )
        polygons->points[p] = new_points[p];

    (void) close_file( file1 );
    (void) close_file( file2 );

    if( output_graphics_file( output_filename, format, n_objects,
                              objects ) != OK )
        return( 1 );

    return( 0 );
}
