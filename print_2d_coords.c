#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               src_polygons_filename;
    File_formats         format;
    int                  n_src_objects, poly;
    object_struct        **src_objects;
    polygons_struct      *polygons, unit_sphere;
    Real                 x, y, z, xs, ys, zs, u, v, dist;
    Point                point, centre, unit_point, poly_point;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_polygons_filename ) ||
        !get_real_argument( 0.0, &x ) ||
        !get_real_argument( 0.0, &y ) ||
        !get_real_argument( 0.0, &z ) )
    {
        print_error( "Usage: %s  input.obj  x y z\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_polygons_filename,
                             &format, &n_src_objects, &src_objects ) != OK )
        return( 1 );

    if( n_src_objects != 1 || get_object_type( src_objects[0] ) != POLYGONS ||
        !is_this_tetrahedral_topology( get_polygons_ptr(src_objects[0]) ) )
    {
        print( "First argument must contain one tetrahedral mesh.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr(src_objects[0]);

    create_polygons_bintree( polygons,
                        ROUND( (Real) polygons->n_items * BINTREE_FACTOR ) );

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0, polygons->n_items,
                               &unit_sphere );

    fill_Point( point, x, y, z );

    poly = find_closest_polygon_point( &point, polygons, &poly_point );

    dist = distance_between_points( &point, &poly_point );

    if( dist > 1.0 )
        print( "Warning point is %.1f millimetres from the surface.\n", dist );

    map_point_to_unit_sphere( polygons, &point, &unit_sphere, &unit_point );

    xs = RPoint_x( unit_point );
    ys = RPoint_y( unit_point );
    zs = RPoint_z( unit_point );

    map_sphere_to_uv( xs, ys, zs, &u, &v );

    print( "%g %g %g -> %g %g\n", x, y, z, u, v );

    delete_object_list( n_src_objects, src_objects );

    delete_polygons( &unit_sphere );

    return( 0 );
}
