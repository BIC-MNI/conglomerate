#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               surface_filename, model_filename;
    STRING               output_filename, dest_filename;
    STRING               input_values_filename, output_values_filename;
    int                  n_objects, p, n_d_objects;
    int                  n_s_objects, poly, size, i;
    Point                centre, poly_point;
    File_formats         format;
    object_struct        **object_list, **s_object_list, *out_object;
    object_struct        **d_object_list;
    Real                 dist, value, *in_values;
    FILE                 *in_file, *out_file;
    polygons_struct      *surface, *dest_sphere, *sphere, *dest_object;
    progress_struct      progress;
    BOOLEAN              values_specified;
    Point                poly1_points[MAX_POINTS_PER_POLYGON];
    Point                poly2_points[MAX_POINTS_PER_POLYGON];
    Point                scaled_point;
    Real                 weights[MAX_POINTS_PER_POLYGON];


    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &model_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  surface.obj surface_model.obj different_model.obj output.obj n\n",
                     argv[0] );
        return( 1 );
    }

    values_specified = get_string_argument( NULL, &input_values_filename ) &&
                       get_string_argument( NULL, &output_values_filename );

    if( input_graphics_file( surface_filename, &format, &n_objects,
                             &object_list ) != OK ||
        n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Must contain exactly one sphere topology object.\n" );
        return( 1 );
    }

    surface = get_polygons_ptr(object_list[0]);

    if( input_graphics_file( model_filename, &format, &n_s_objects,
                             &s_object_list ) != OK ||
        n_s_objects != 1 || get_object_type(s_object_list[0]) != POLYGONS )
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

    if( input_graphics_file( dest_filename, &format, &n_d_objects,
                             &d_object_list ) != OK ||
        n_d_objects != 1 || get_object_type(d_object_list[0]) != POLYGONS )
    {
        print_error( "Must contain exactly one sphere topology object.\n" );
        return( 1 );
    }

    dest_object = get_polygons_ptr(d_object_list[0]);

    out_object = create_object( POLYGONS );
    dest_sphere = get_polygons_ptr( out_object );
    copy_polygons( dest_object, dest_sphere );

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


    create_polygons_bintree( sphere,
                             ROUND( (Real) sphere->n_items * 0.3 ) );

    initialize_progress_report( &progress, FALSE, dest_sphere->n_points,
                                "Mapping" );
    for_less( p, 0, dest_sphere->n_points )
    {
        poly = find_closest_polygon_point( &dest_object->points[p],
                                           sphere, &poly_point );

        dist = distance_between_points( &poly_point, &dest_sphere->points[p] );

        if( dist > 0.01 )
            print( "%d:  %g\n", p, dist );
        
        size = get_polygon_points( sphere, poly, poly1_points );

        get_polygon_interpolation_weights( &poly_point, size, poly1_points,
                                           weights );

        if( get_polygon_points( surface, poly, poly2_points ) != size )
            handle_internal_error( "map_point_between_polygons" );

        fill_Point( dest_sphere->points[p], 0.0, 0.0, 0.0 );
        if( values_specified )
            value = 0.0;

        for_less( i, 0, size )
        {
            SCALE_POINT( scaled_point, poly2_points[i], weights[i] );
            ADD_POINTS( dest_sphere->points[p], dest_sphere->points[p],
                        scaled_point );
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

    compute_polygon_normals( dest_sphere );

    if( output_graphics_file( output_filename, format, 1, &out_object ) != OK )
        return( 1 );

    return( 0 );
}
