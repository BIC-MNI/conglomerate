#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               surface_filename, unit_sphere_filename;
    STRING               output_filename;
    int                  n_objects, n_triangles, p;
    int                  n_s_objects, poly;
    Point                centre, *new_points, poly_point;
    File_formats         format;
    object_struct        **object_list, **s_object_list, *out_object;
    Real                 dist;
    polygons_struct      *surface, *dest_sphere, *sphere;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &unit_sphere_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &n_triangles ) )
    {
        print_error( "Usage: %s  surface.obj sphere.obj output.obj n\n",
                     argv[0] );
        return( 1 );
    }

    if( input_graphics_file( surface_filename, &format, &n_objects,
                             &object_list ) != OK ||
        n_objects != 1 || get_object_type(object_list[0]) != POLYGONS ||
        !is_this_tetrahedral_topology(get_polygons_ptr(object_list[0])) )
    {
        print_error( "Must contain exactly one sphere topology object.\n" );
        return( 1 );
    }

    surface = get_polygons_ptr(object_list[0]);

    if( input_graphics_file( unit_sphere_filename, &format, &n_s_objects,
                             &s_object_list ) != OK ||
        n_s_objects != 1 || get_object_type(s_object_list[0]) != POLYGONS ||
        !is_this_tetrahedral_topology(get_polygons_ptr(s_object_list[0])) )
    {
        print_error( "Must contain exactly one sphere topology object.\n" );
        return( 1 );
    }

    sphere = get_polygons_ptr(s_object_list[0]);

    if( surface->n_items != sphere->n_items )
    {
        print_error( "Topologies do not match\n" );
        return( 1 );
    }

    fill_Point( centre, 0.0, 0.0, 0.0 );

    out_object = create_object( POLYGONS );
    dest_sphere = get_polygons_ptr( out_object );
    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               n_triangles, dest_sphere );

    create_polygons_bintree( sphere,
                             ROUND( (Real) sphere->n_items * 0.7 ) );

    ALLOC( new_points, dest_sphere->n_points );
    initialize_progress_report( &progress, FALSE, dest_sphere->n_points,
                                "Mapping" );
    for_less( p, 0, dest_sphere->n_points )
    {
        poly = find_closest_polygon_point( &dest_sphere->points[p],
                                           sphere, &poly_point );

        dist = distance_between_points( &poly_point, &dest_sphere->points[p] );

        if( dist > 0.001 )
            print( "%d:  %g\n", p, dist );

        map_point_between_polygons( sphere, poly, &poly_point, surface,
                                    &new_points[p] );

        update_progress_report( &progress, p+1 );
    }

    terminate_progress_report( &progress );

    for_less( p, 0, dest_sphere->n_points )
        dest_sphere->points[p] = new_points[p];

    compute_polygon_normals( dest_sphere );

    if( output_graphics_file( output_filename, format, 1, &out_object ) != OK )
        return( 1 );

    return( 0 );
}
