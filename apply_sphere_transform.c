#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

#define  MIN_INTERP .45
#define  MAX_INTERP .55

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  surface.obj  u.txt v.txt  output.obj [alternate]\n\
\n\
     .\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING              surface_filename, output_filename;
    STRING              u1_filename, v1_filename;
    STRING              u2_filename, v2_filename;
    File_formats        format;
    int                 n_objects, p;
    Real                u, v, u_off, v_off, x, y, z, t1;
    Real                len;
    FILE                *u1_file, *u2_file, *v1_file, *v2_file;
    Point               centre, unit_point, *new_points, trans_point;
    Point               trans_point2;
    polygons_struct     *polygons, unit_sphere;
    object_struct       **objects;
    BOOLEAN             interp_flag;
    Real                ratio;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &u1_filename ) ||
        !get_string_argument( NULL, &v1_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    interp_flag = get_string_argument( NULL, &u2_filename ) &&
                  get_string_argument( NULL, &v2_filename );

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

    create_polygons_bintree( &unit_sphere,
                             ROUND( (Real) unit_sphere.n_items * 0.2 ) );

    if( open_file( u1_filename, READ_FILE, ASCII_FORMAT, &u1_file ) != OK ||
        open_file( v1_filename, READ_FILE, ASCII_FORMAT, &v1_file ) != OK )
        return( 1 );

    if( interp_flag )
    {
        if( open_file( u2_filename, READ_FILE, ASCII_FORMAT, &u2_file ) != OK ||
            open_file( v2_filename, READ_FILE, ASCII_FORMAT, &v2_file ) != OK )
            return( 1 );
    }

    ALLOC( new_points, polygons->n_points );

    for_less( p, 0, polygons->n_points )
    {
        if( input_real( u1_file, &u_off ) != OK ||
            input_real( v1_file, &v_off ) != OK )
        {
            print_error( "Error reading u and v.\n" );
            return( 1 );
        }

        map_point_to_unit_sphere( polygons, &polygons->points[p],
                                  &unit_sphere, &unit_point );

        x = RPoint_x( unit_point );
        y = RPoint_y( unit_point );
        z = RPoint_z( unit_point );

        map_sphere_to_uv( x, y, z, &u, &v );

        t1 = 2.0 * FABS( (v - 0.5) );

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

        fill_Point( trans_point, x, y, z );

        if( interp_flag )
        {
            if( input_real( u2_file, &u_off ) != OK ||
                input_real( v2_file, &v_off ) != OK )
            {
                print_error( "Error reading u and v.\n" );
                return( 1 );
            }

            map_point_to_unit_sphere( polygons, &polygons->points[p],
                                      &unit_sphere, &unit_point );

            x = -RPoint_z( unit_point );
            y = RPoint_y( unit_point );
            z = RPoint_x( unit_point );

            map_sphere_to_uv( x, y, z, &u, &v );

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

            fill_Point( trans_point2, z, y, -x );

            if( t1 <= MIN_INTERP )
                ratio = 0.0;
            else if( t1 >= MAX_INTERP )
                ratio = 1.0;
            else
                ratio = (t1 - MIN_INTERP) / (MAX_INTERP - MIN_INTERP);

            INTERPOLATE_POINTS( trans_point, trans_point, trans_point2, ratio );

            len = DOT_POINTS( trans_point, trans_point );
            len = sqrt( len );
            SCALE_POINT( trans_point, trans_point, 1.0 / len );
        }

        map_unit_sphere_to_point( &unit_sphere, &trans_point,
                                  polygons, &new_points[p] );
    }

    for_less( p, 0, polygons->n_points )
        polygons->points[p] = new_points[p];

    (void) close_file( u1_file );
    (void) close_file( v1_file );

    if( interp_flag )
    {
        (void) close_file( u2_file );
        (void) close_file( v2_file );
    }

    if( output_graphics_file( output_filename, format, n_objects,
                              objects ) != OK )
        return( 1 );

    return( 0 );
}
