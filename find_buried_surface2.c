#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               src_polygons_filename, dest_polygons_filename;
    STRING               input_filename, output_filename;
    File_formats         format;
    int                  n_src_objects, poly, obj_index, size;
    Real                 radius_of_curvature, dist;
    object_struct        **src_objects;
    polygons_struct      *polygons;
    BOOLEAN              on_boundary;
    Point                centroid, sphere_centre, *points;
    Point                found_point;
    Vector               normal;
    progress_struct      progress;
    Real                 surface_area, buried_surface_area, total_surface_area;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_polygons_filename ) ||
        !get_string_argument( NULL, &dest_polygons_filename ) ||
        !get_real_argument( 0.0, &radius_of_curvature ) )
    {
        print_error(
             "Usage: %s  src_polygons  dest_polygons radius_of_curvature\n",
               argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_polygons_filename,
                             &format, &n_src_objects, &src_objects ) != OK )
        return( 1 );

    if( n_src_objects < 1 || get_object_type( src_objects[0] ) != POLYGONS )
    {
        print( "Must specify polygons file.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr(src_objects[0]);

    set_polygon_per_item_colours( polygons );

    create_polygons_bintree( polygons,
                         ROUND( (Real) polygons->n_items * BINTREE_FACTOR ) );

    ALLOC( points, polygons->n_points );

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Pseudo-convex hulling" );

    total_surface_area = 0.0;
    buried_surface_area = 0.0;

    for_less( poly, 0, polygons->n_items )
    {
        size = get_polygon_points( polygons, poly, points );

        get_points_centroid( size, points, &centroid );
        find_polygon_normal( size, points, &normal );

        GET_POINT_ON_RAY( sphere_centre, centroid, normal,
                          radius_of_curvature );

        dist = find_closest_point_on_object( &sphere_centre, src_objects[0],
                                             &obj_index, &found_point );

        on_boundary = (obj_index == poly);

        if( !on_boundary )
        {
            if( dist > radius_of_curvature * 1.0001 )
                handle_internal_error( "dist > radius_of_curvature * 1.0001" );
        }
        
        if( on_boundary )
            polygons->colours[poly] = WHITE;
        else
            polygons->colours[poly] = RED;

        surface_area = get_polygon_surface_area( size, points );

        total_surface_area += surface_area;
        if( !on_boundary )
            buried_surface_area += surface_area;

        update_progress_report( &progress, poly+1 );
    }

    terminate_progress_report( &progress );

    print( "Percent buried: %g\n", 100.0 *
           buried_surface_area / total_surface_area );

    (void) output_graphics_file( dest_polygons_filename,
                                 format, n_src_objects, src_objects );

    delete_object_list( n_src_objects, src_objects );

    FREE( points  );

    return( 0 );
}
