#include <volume_io/internal_volume_io.h>
#include <bicpl.h>

private  void  subdivide_big_polygons(
    polygons_struct    *polygons,
    Real               max_length );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  output.obj  max_edge_length\n\
\n\
     Subdivides any polygons in the file, placing output in the original file\n\
     or in a different output file.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, output_filename;
    int              i, n_objects;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons;
    Real             max_length;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &max_length ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( equal_strings( output_filename, "-" ) )
        output_filename = input_filename;

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type(object_list[i]) == POLYGONS )
        {
            polygons = get_polygons_ptr( object_list[i] );

            subdivide_big_polygons( polygons, max_length );

            compute_polygon_normals( polygons );
        }
    }

    (void) output_graphics_file( output_filename, format,
                                 n_objects, object_list );

    delete_object_list( n_objects, object_list );

    return( 0 );
}

private  BOOLEAN  polygon_should_be_subdivided(
    polygons_struct    *polygons,
    int                poly,
    Real               max_length )
{
    int   vertex, p1, p2;

    size = GET_OBJECT_SIZE( *polygons, poly );

    p2 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,size-1)];

    for_less( vertex, 0, size )
    {
        p1 = p2;
        p2 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,vertex)];

        dist = sq_distance_between_points( &polygons->points[p1],
                                           &polygons->points[p2] );

        if( dist > max_length * max_length )
            return( TRUE );
    }

    return( FALSE );
}

private  void  subdivide_big_polygons(
    polygons_struct    *polygons,
    Real               max_length )
{
    int                i, new_n_points, new_n_indices, new_n_polygons;
    int                *new_indices, *new_end_indices;
    int                size, *n_point_neighbours, **point_neighbours;
    int                **midpoints;
    int                total_n_point_neighbours;
    Point              *new_points;
    Colour             save_colour;
    progress_struct    progress;

    save_colour = polygons->colours[0];

    new_n_points = polygons->n_points;
    new_n_polygons = 0;
    new_n_indices = 0;

    new_points = NULL;
    size = 0;
    SET_ARRAY_SIZE( new_points, size, new_n_points, DEFAULT_CHUNK_SIZE );

    for_less( i, 0, new_n_points )
        new_points[i] = polygons->points[i];

    create_polygon_point_neighbours( polygons, FALSE, &n_point_neighbours,
                                     &point_neighbours, NULL, NULL );

    total_n_point_neighbours = 0;

    for_less( i, 0, new_n_points )
        total_n_point_neighbours += n_point_neighbours[i];

    ALLOC( midpoints, new_n_points );
    ALLOC( midpoints[0], total_n_point_neighbours );

    for_less( i, 1, new_n_points )
        midpoints[i] = &midpoints[i-1][n_point_neighbours[i-1]];

    for_less( i, 0, total_n_point_neighbours )
        midpoints[0][i] = -1;

    initialize_progress_report( &progress, TRUE, polygons->n_items,
                                "Making Edge Midpoints" );

    for_less( i, 0, polygons->n_items )
    {
        if( polygon_should_be_subdivided( polygons, i, max_length ) )
        {
            for_less( 
            subdivide_polygon( polygons, i,
                               &new_n_points, &new_points,
                               &new_n_polygons, &new_end_indices,
                               &new_n_indices, &new_indices,
                               n_point_neighbours, point_neighbours, midpoints);
        }
        update_progress_report( &progress, i+1 );
    }

    terminate_progress_report( &progress );

    initialize_progress_report( &progress, TRUE, polygons->n_items,
                                "Subdividing Polygon" );

    for_less( i, 0, polygons->n_items )
    {
        if( polygon_should_be_subdivided( polygons, i, max_length ) )
        {
            subdivide_polygon( polygons, i,
                               &new_n_points, &new_points,
                               &new_n_polygons, &new_end_indices,
                               &new_n_indices, &new_indices,
                               n_point_neighbours, point_neighbours, midpoints);
        }
        update_progress_report( &progress, i+1 );
    }

    terminate_progress_report( &progress );

    delete_polygon_point_neighbours( polygons, n_point_neighbours,
                                     point_neighbours, NULL, NULL );

    FREE( midpoints[0] );
    FREE( midpoints );

    delete_polygons( polygons );

    ALLOC( polygons->colours, 1 );
    polygons->colour_flag = ONE_COLOUR;
    polygons->colours[0] = save_colour;

    ALLOC( polygons->normals, new_n_points );

    polygons->n_points = new_n_points;
    polygons->points = new_points;
    polygons->n_items = new_n_polygons;
    polygons->end_indices = new_end_indices;
    polygons->indices = new_indices;
}
