#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.4

private  void  scan_lines_to_polygons(
    polygons_struct    *polygons,
    int                n_neighbours[],
    int                *neighbours[],
    Real               vertex_values[],
    Real               distance,
    Real               scan_step,
    lines_struct       *lines );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               polygons_filename, input_filename;
    STRING               output_filename;
    File_formats         format;
    int                  i, point, *n_neighbours, **neighbours;
    int                  n_polygon_objects, n_objects;
    object_struct        **objects, **polygon_objects;
    polygons_struct      *polygons;
    Real                 distance, scan_step, *vertex_values;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &polygons_filename ) ||
        !get_real_argument( 0.0, &distance ) ||
        !get_real_argument( 0.0, &scan_step ) )
    {
        print_error(
             "Usage: %s  polygons.obj  dist scan_step\n", argv[0] );
        print_error( "        [input1.obj output1.mnc] [input2.obj output2.mnc]\n" );
        return( 1 );
    }

    if( input_graphics_file( polygons_filename, &format,
                             &n_polygon_objects, &polygon_objects ) != OK ||
        n_polygon_objects < 1 ||
        get_object_type( polygon_objects[0] ) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( polygon_objects[0] );

    create_polygon_point_neighbours( polygons, TRUE,
                                     &n_neighbours, &neighbours, NULL, NULL );

    create_polygons_bintree( polygons,
                             ROUND((Real) polygons->n_items * BINTREE_FACTOR ));

    ALLOC( vertex_values, polygons->n_points );

    while( get_string_argument( NULL, &input_filename ) &&
           get_string_argument( NULL, &output_filename ) )
    {
        if( input_graphics_file( input_filename,
                                 &format, &n_objects, &objects ) != OK )
            return( 1 );

        for_less( point, 0, polygons->n_points )
            vertex_values[point] = 0.0;

        for_less( i, 0, n_objects )
        {
            if( get_object_type( objects[i] ) == LINES )
            {
                scan_lines_to_polygons( polygons, n_neighbours, neighbours,
                                        vertex_values, distance, scan_step, 
                                        get_lines_ptr(objects[i]) );
            }
        }

        if( output_texture_values( output_filename, BINARY_FORMAT,
                                   polygons->n_points, vertex_values ) != OK )
            return( 1 );

        delete_object_list( n_objects, objects );
    }

    delete_object_list( n_polygon_objects, polygon_objects );

    return( 0 );
}

private  void  scan_lines_to_polygons(
    polygons_struct    *polygons,
    int                n_neighbours[],
    int                *neighbours[],
    Real               vertex_values[],
    Real               scan_distance,
    Real               scan_step,
    lines_struct       *lines )
{
    int         point, line, size, v, p1, p2, n_steps, n_to_do;
    int         step, poly, poly_size, vertex, poly_vertex, neigh, current, n;
    Real        ratio, interval, priority;
    Point       p, polygon_point;
    float       *distances, dist, new_dist;
    PRIORITY_QUEUE_STRUCT( int )  queue;

    ALLOC( distances, polygons->n_points );
    for_less( point, 0, polygons->n_points )
        distances[point] = -1.0f;

    for_less( line, 0, lines->n_items )
    {
        size = GET_OBJECT_SIZE( *lines, line );
        for_less( v, 0, size-1 )
        {
            p1 = lines->indices[POINT_INDEX(lines->end_indices,line,v)];
            p2 = lines->indices[POINT_INDEX(lines->end_indices,line,v+1)];

            interval = distance_between_points( &lines->points[p1],
                                            &lines->points[p2] );

            n_steps = ROUND( interval / scan_step );
            if( v < size-2 )
                n_to_do = n_steps;
            else
                n_to_do = n_steps+1;

            for_less( step, 0, n_to_do )
            {
                ratio = (Real) step / (Real) n_steps;
                INTERPOLATE_POINTS( p, lines->points[p1], lines->points[p2],
                                    ratio );

                poly = find_closest_polygon_point( &p, polygons,
                                                   &polygon_point );

                poly_size = GET_OBJECT_SIZE( *polygons, poly );
                for_less( vertex, 0, poly_size )
                {
                    poly_vertex = polygons->indices[POINT_INDEX(
                                   polygons->end_indices,poly,vertex)];
                    dist = (float) distance_between_points(
                                    &polygons->points[poly_vertex],
                                    &polygon_point );

                    if( dist <= (float) scan_distance &&
                        (distances[poly_vertex] < 0.0f ||
                         dist < distances[poly_vertex]) )
                        distances[poly_vertex] = dist;
                }
            }
        }
    }

    INITIALIZE_PRIORITY_QUEUE( queue );

    for_less( poly_vertex, 0, polygons->n_points )
    {
        if( distances[poly_vertex] >= 0.0f )
        {
            INSERT_IN_PRIORITY_QUEUE( queue, poly_vertex,
                                      (Real) -distances[poly_vertex] );
        }
    }

    while( !IS_PRIORITY_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_PRIORITY_QUEUE( queue, current, priority );

        priority = -priority;

        if( priority > (Real) distances[current] * 1.01 )
            continue;

        for_less( n, 0, n_neighbours[current] )
        {
            neigh = neighbours[current][n];
            new_dist = distances[current] + (float) distance_between_points(
                                         &polygons->points[current],
                                         &polygons->points[neigh] );

            if( new_dist <= (float) scan_distance &&
                (distances[neigh] < 0.0f || new_dist < distances[neigh]) )
            {
                distances[neigh] = new_dist;
                INSERT_IN_PRIORITY_QUEUE( queue, neigh,
                                          (Real) -distances[neigh] );
            }
        }
    }

    DELETE_PRIORITY_QUEUE( queue );

    for_less( poly_vertex, 0, polygons->n_points )
    {
        if( distances[poly_vertex] >= 0.0f )
            vertex_values[poly_vertex] = 1.0;
    }

    FREE( distances );
}
