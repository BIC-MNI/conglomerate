#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR               surface_filename, unit_sphere_filename;
    VIO_STR               output_filename;
    VIO_STR               input_values_filename, output_values_filename;
    int                  n_objects, n_triangles, p;
    int                  n_s_objects, poly, size, i;
    VIO_Point                centre, *new_points, poly_point;
    VIO_File_formats         format;
    object_struct        **object_list, **s_object_list, *out_object;
    VIO_Real                 dist, value, *in_values;
    FILE                 *in_file, *out_file;
    polygons_struct      *surface, *dest_sphere, *sphere;
    VIO_progress_struct      progress;
    BOOLEAN              values_specified;
    VIO_Point                poly1_points[MAX_POINTS_PER_POLYGON];
    VIO_Point                poly2_points[MAX_POINTS_PER_POLYGON];
    VIO_Point                scaled_point;
    VIO_Real                 weights[MAX_POINTS_PER_POLYGON];


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

    values_specified = get_string_argument( NULL, &input_values_filename ) &&
                       get_string_argument( NULL, &output_values_filename );

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
                             VIO_ROUND( (VIO_Real) sphere->n_items * 0.7 ) );

    ALLOC( new_points, dest_sphere->n_points );

    if( values_specified )
    {
        ALLOC( in_values, surface->n_points );
        if( open_file( input_values_filename, READ_FILE, ASCII_FORMAT,
                       &in_file ) != OK )
            return( 1 );

        for_less( p, 0, surface->n_points )
        {
            if( input_real( in_file, &in_values[p] ) != OK )
            {
                print_error( "Error reading values.\n" );
                return( 1 );
            }
        }

        (void) close_file( in_file );

        if( open_file( output_values_filename, WRITE_FILE, ASCII_FORMAT,
                       &out_file ) != OK )
            return( 1 );
    }

    initialize_progress_report( &progress, FALSE, dest_sphere->n_points,
                                "Mapping" );
    for_less( p, 0, dest_sphere->n_points )
    {
        poly = find_closest_polygon_point( &dest_sphere->points[p],
                                           sphere, &poly_point );

        dist = distance_between_points( &poly_point, &dest_sphere->points[p] );

        if( dist > 0.01 )
            print( "%d:  %g\n", p, dist );
        
        size = get_polygon_points( sphere, poly, poly1_points );

        get_polygon_interpolation_weights( &poly_point, size, poly1_points,
                                           weights );

        if( get_polygon_points( surface, poly, poly2_points ) != size )
            handle_internal_error( "map_point_between_polygons" );

        fill_Point( new_points[p], 0.0, 0.0, 0.0 );
        if( values_specified )
            value = 0.0;

        for_less( i, 0, size )
        {
            SCALE_POINT( scaled_point, poly2_points[i], weights[i] );
            ADD_POINTS( new_points[p], new_points[p], scaled_point );
            if( values_specified )
                value += weights[i] * in_values[surface->indices[
                            POINT_INDEX(surface->end_indices,poly,i)]];
        }

        if( values_specified )
        {
            if( output_real( out_file, value ) != OK ||
                output_newline( out_file ) != OK )
            {
                print_error( "Error writing values.\n" );
                return( 1 );
            }
        }

        update_progress_report( &progress, p+1 );
    }

    terminate_progress_report( &progress );

    if( values_specified )
        close_file( out_file );

    for_less( p, 0, dest_sphere->n_points )
        dest_sphere->points[p] = new_points[p];

    compute_polygon_normals( dest_sphere );

    if( output_graphics_file( output_filename, format, 1, &out_object ) != OK )
        return( 1 );

    return( 0 );
}
