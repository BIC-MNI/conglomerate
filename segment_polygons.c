#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  void  segment_polygons(
    polygons_struct   *polygons,
    lines_struct      *lines,
    polygons_struct   *output_polygons );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               polygons_filename, dest_filename, lines_filename;
    int                  n_objects, n_l_objects;
    File_formats         format;
    object_struct        **object_list, **l_object_list;
    polygons_struct      *polygons, *new_polygons;
    lines_struct         *lines;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &polygons_filename ) ||
        !get_string_argument( NULL, &lines_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  polygons.obj lines.obj output.obj\n",
                     argv[0] );
        return( 1 );
    }

    if( input_graphics_file( polygons_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( lines_filename, &format, &n_l_objects,
                             &l_object_list ) != OK || n_l_objects != 1 ||
        get_object_type(l_object_list[0]) != LINES )
        return( 1 );

    lines = get_lines_ptr( l_object_list[0] );

    new_polygons = NULL;
    segment_polygons( polygons, lines, new_polygons );

    (void) output_graphics_file( dest_filename, format, n_objects, object_list);

    return( 0 );
}

private  void  segment_polygons(
    polygons_struct   *polygons,
    lines_struct      *lines,
    polygons_struct   *output_polygons )
{
    int           *line_index, p, l, best, poly, size, edge, p1, p2;
    int           n_done, current, neigh;
    QUEUE_STRUCT( int )  queue;
    Smallest_int  *flags;
    Real          dist, best_dist;

    check_polygons_neighbours_computed( polygons );

    ALLOC( line_index, polygons->n_points );
    for_less( p, 0, polygons->n_points )
        line_index[p] = -1;
    for_less( l, 0, lines->n_points )
    {
        best = 0;
        best_dist = 0.0;
        for_less( p, 0, polygons->n_points )
        {
            dist = sq_distance_between_points( &polygons->points[p],
                                               &lines->points[l] );
            if( p == 0 || dist < best_dist )
            {
                best_dist = dist;
                best = p;
            }
        }

        if( best_dist > 0.0 )
            print_error( "Dist: %g\n", best_dist );


        line_index[best] = l;
    }

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( edge, 0, size )
        {
            p1 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                               edge)];
            p2 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                               (edge+1)%size)];

            if( line_index[p1] < 0 || line_index[p2] < 0 )
                continue;

            if( line_index[p1] != line_index[p2]+1 &&
                line_index[p1] != line_index[p2]-1 )
            {
/*
                handle_internal_error( "line_index" );
*/
                continue;
            }

            polygons->neighbours[POINT_INDEX(polygons->end_indices,poly,edge)]
                                            = -1;
        }
    }

    FREE( line_index );

    ALLOC( flags, polygons->n_items );
    for_less( poly, 0, polygons->n_items )
        flags[poly] = 0;

    INITIALIZE_QUEUE( queue );
    flags[0] = 1;
    n_done = 1;
    INSERT_IN_QUEUE( queue, 0 );

    while( !IS_QUEUE_EMPTY(queue) )
    {
        REMOVE_FROM_QUEUE( queue, current );

        size = GET_OBJECT_SIZE( *polygons, current );
        for_less( edge, 0, size )
        {
            neigh = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                                     current,edge)];
            if( neigh >= 0 && flags[neigh] == 0 )
            {
                flags[neigh] = 1;
                ++n_done;
                INSERT_IN_QUEUE( queue, neigh );
            }
        }
    }

    DELETE_QUEUE( queue );
    FREE( flags );

    print( "Did %d out of %d\n", n_done, polygons->n_items );
}
