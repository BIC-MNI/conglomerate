#include  <volume_io/internal_volume_io.h>
#include  <deform.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE                        *file;
    STRING                      volume_filename, surface_filename;
    STRING                      output_filename;
    int                         p, n_objects;
    File_formats                format;
    object_struct               **object_list;
    polygons_struct             *polygons;
    Volume                      volume;
    Real                        dist, threshold;
    Real                        max_inwards_distance, max_outwards_distance;
    boundary_definition_struct  boundary_def;
    progress_struct             progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &surface_filename ) ||
        !get_real_argument( 0.0, &threshold ) ||
        !get_real_argument( 0.0, &max_inwards_distance ) ||
        !get_real_argument( 0.0, &max_outwards_distance ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
                 "Usage: %s  volume.mnc surface.obj thresh max_in max_out out.txt\n",
                 argv[0] );
        return( 1 );
    }

    if( input_graphics_file( surface_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "Error reading surface.\n" );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    compute_polygon_normals( polygons );

    set_boundary_definition( &boundary_def, threshold, threshold, 0.0, 0.0,
                             ' ', 1.0e-10 );

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Searching for boundaries" );

    for_less( p, 0, polygons->n_points )
    {
        if( !find_boundary_in_direction( volume, NULL, NULL, NULL, NULL,
                                         0.0, &polygons->points[p],
                                         &polygons->normals[p],
                                         &polygons->normals[p],
                                         max_outwards_distance,
                                         max_inwards_distance, 0,
                                         &boundary_def, &dist ) )
            dist = 2.0 * max_outwards_distance;

        if( output_real( file, dist ) != OK ||
            output_newline( file ) != OK )
            return( 1 );

        update_progress_report( &progress, p + 1 );
    }

    terminate_progress_report( &progress );

    close_file( file );

    return( 0 );
}
