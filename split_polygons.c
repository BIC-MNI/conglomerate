#include <internal_volume_io.h>
#include <bicpl.h>

private  int   split_polygons(
    polygons_struct    *polygons,
    polygons_struct    *out1,
    polygons_struct    *out2,
    int                translation1[],
    int                translation2[] );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  output1.obj output2.obj [translation.out]\n\
\n\
     Splits polygons into two separate halves.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input_filename, output1_filename, output2_filename;
    STRING           translation_filename;
    int              n_objects, *translation1, *translation2;
    int              n_in_half, p;
    File_formats     format;
    object_struct    **object_list, *object1, *object2;
    polygons_struct  *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output1_filename ) ||
        !get_string_argument( NULL, &output2_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_string_argument( NULL, &translation_filename );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects < 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "File must have a polygons structure.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );
    check_polygons_neighbours_computed( polygons );

    object1 = create_object( POLYGONS );
    initialize_polygons( get_polygons_ptr(object1), WHITE, NULL );
    object2 = create_object( POLYGONS );
    initialize_polygons( get_polygons_ptr(object2), WHITE, NULL );

    ALLOC( translation1, polygons->n_points );
    ALLOC( translation2, polygons->n_points );

    n_in_half = split_polygons( polygons, get_polygons_ptr(object1),
                                get_polygons_ptr(object2),
                                translation1, translation2 );

    print( "Split into %d and %d polygons.\n", n_in_half,
           polygons->n_items - n_in_half );

    (void) output_graphics_file( output1_filename, format,
                                 1, &object1 );
    (void) output_graphics_file( output2_filename, format,
                                 1, &object2 );

    if( translation_filename != NULL )
    {
        if( open_file( translation_filename, WRITE_FILE, BINARY_FORMAT,
                       &file ) != OK )
            return( 1 );

        for_less( p, 0, get_polygons_ptr(object1)->n_points )
        {
            if( io_int( file, WRITE_FILE, BINARY_FORMAT, &translation1[p] ) !=
                        OK )
                return( 1 );
        }

        for_less( p, 0, get_polygons_ptr(object2)->n_points )
        {
            if( io_int( file, WRITE_FILE, BINARY_FORMAT, &translation2[p] ) !=
                        OK )
                return( 1 );
        }

        (void) close_file( file );
    }

    return( 0 );
}

private  BOOLEAN   can_include(
    polygons_struct  *polygons,
    int              target_poly,
    Smallest_int     included[],
    Smallest_int     connected[] )
{
    int                n_neighbours, neighbours[10000];
    int                poly, neigh_poly, size, edge, p, n, n_connected;
    QUEUE_STRUCT(int)  queue;

    size = GET_OBJECT_SIZE( *polygons, target_poly );

    n_neighbours = 0;
    for_less( edge, 0, size )
    {
        neigh_poly = polygons->neighbours[
                         POINT_INDEX(polygons->end_indices,target_poly,edge)];
        if( neigh_poly >= 0 && !included[neigh_poly] )
        {
            neighbours[n_neighbours] = neigh_poly;
            ++n_neighbours;
        }
    }

    if( n_neighbours < 0 )
        return( TRUE );

    for_less( p, 0, polygons->n_items )
        connected[p] = (Smallest_int) FALSE;

    included[target_poly] = (Smallest_int) TRUE;

    INITIALIZE_QUEUE( queue );

    neigh_poly = neighbours[0];
    connected[neigh_poly] = (Smallest_int) TRUE;
    INSERT_IN_QUEUE( queue, neigh_poly );
    n_connected = 1;

    while( n_connected < n_neighbours && !IS_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_QUEUE( queue, poly );

        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( edge, 0, size )
        {
            neigh_poly = polygons->neighbours[
                             POINT_INDEX(polygons->end_indices,poly,edge)];

            if( neigh_poly >= 0 &&
                !included[neigh_poly] && !connected[neigh_poly] )
            {
                for_less( n, 1, n_neighbours )
                {
                    if( neigh_poly == neighbours[n] )
                        ++n_connected;
                }

                INSERT_IN_QUEUE( queue, neigh_poly );
                connected[neigh_poly] = (Smallest_int) TRUE;
            }
        }
    }

    DELETE_QUEUE( queue );

    included[target_poly] = (Smallest_int) FALSE;

    return( n_connected == n_neighbours );
}

private  int  count_neighbours_included(
    polygons_struct    *polygons,
    int                poly,
    Smallest_int       included[],
    int                *n_borders )
{
    int      size, neigh_poly, vertex, n_polys, p, n_included, polys[10000];
    BOOLEAN  closed_flag;

    n_included = 0;

    *n_borders = 0;
    size = GET_OBJECT_SIZE( *polygons, poly );
    for_less( vertex, 0, size )
    {
        neigh_poly = polygons->neighbours[
                        POINT_INDEX(polygons->end_indices,poly,vertex)];
        if( neigh_poly >= 0 && included[neigh_poly] )
            ++(*n_borders);

        n_polys = get_polygons_around_vertex( polygons, poly, vertex,
                                              polys, 10000, &closed_flag );

        for_less( p, 0, n_polys )
        {
            if( included[polys[p]] )
                ++n_included;
        }
    }

    return( n_included - *n_borders );
}

private  int   assign_included(
    polygons_struct    *polygons,
    Smallest_int       included[] )
{
    int                         init_poly, poly, neigh_poly, size;
    int                         n_included, edge, n_neighbours_included;
    int                         n_borders;
    Real                        priority, next_priority;
    Smallest_int                *connected;
    BOOLEAN                     found;
    PRIORITY_QUEUE_STRUCT(int)  queue;
    progress_struct             progress;

    found = FALSE;
    init_poly = -1;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( edge, 0, size )
        {
            neigh_poly = polygons->neighbours[
                        POINT_INDEX(polygons->end_indices,poly,edge)];

            if( neigh_poly < 0 )
            {
                found = TRUE;
                init_poly = poly;
                break;
            }
        }

        if( found )
            break;
    }

    if( !found )
        init_poly = 0;

    ALLOC( connected, polygons->n_items );

    for_less( poly, 0, polygons->n_items )
        included[poly] = (Smallest_int) FALSE;

    INITIALIZE_PRIORITY_QUEUE( queue );
    INSERT_IN_PRIORITY_QUEUE( queue, init_poly, -0.5 );
    included[init_poly] = (Smallest_int) TRUE;
    n_included = 1;

    initialize_progress_report( &progress, FALSE, polygons->n_items / 2,
                                "Splitting" );

    while( n_included < polygons->n_items / 2 &&
           !IS_PRIORITY_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_PRIORITY_QUEUE( queue, poly, priority );
        priority = -priority;

        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( edge, 0, size )
        {
            neigh_poly = polygons->neighbours[
                             POINT_INDEX(polygons->end_indices,poly,edge)];

            if( neigh_poly >= 0 &&
                !included[neigh_poly] &&
                can_include( polygons, neigh_poly, included, connected ) )
            {
                n_neighbours_included = count_neighbours_included(
                                             polygons, neigh_poly, included,
                                             &n_borders );
                next_priority = (Real) ((int) priority + 1) + 1.0 -
                                (Real) n_borders / 100.0 -
                                (Real) n_neighbours_included / 10000.0;
                INSERT_IN_PRIORITY_QUEUE( queue, neigh_poly,
                                          -next_priority );
                included[neigh_poly] = (Smallest_int) TRUE;
                ++n_included;
                update_progress_report( &progress, n_included - 1 );
            }
        }
    }

    terminate_progress_report( &progress );

    DELETE_PRIORITY_QUEUE( queue );

    FREE( connected );

    return( n_included );
}

private  int   split_polygons(
    polygons_struct    *polygons,
    polygons_struct    *out1,
    polygons_struct    *out2,
    int                translation1[],
    int                translation2[] )
{
    int                p, ind, poly, vertex, size, point;
    int                n_included, n_indices[2];
    Smallest_int       *included;
    polygons_struct    *out[2];
    int                *translation[2];

    ALLOC( included, polygons->n_items );

    n_included = assign_included( polygons, included );

    ALLOC( translation[0], polygons->n_items );
    ALLOC( translation[1], polygons->n_items );
    for_less( p, 0, polygons->n_points )
    {
        translation[0][p] = -1;
        translation[1][p] = -1;
    }

    out[0] = out1;
    out[1] = out2;
    n_indices[0] = 0;
    n_indices[1] = 0;

    for_less( poly, 0, polygons->n_items )
    {
        if( included[poly] )
            ind = 0;
        else
            ind = 1;

        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( vertex, 0, size )
        {
            point = polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                                  vertex)];

            if( translation[ind][point] < 0 )
            {
                translation[ind][point] = out[ind]->n_points;
                ADD_ELEMENT_TO_ARRAY( out[ind]->points, out[ind]->n_points,
                                      polygons->points[point],
                                      DEFAULT_CHUNK_SIZE );
                --out[ind]->n_points;
                ADD_ELEMENT_TO_ARRAY( out[ind]->normals, out[ind]->n_points,
                                      polygons->normals[point],
                                      DEFAULT_CHUNK_SIZE );
            }
            ADD_ELEMENT_TO_ARRAY( out[ind]->indices, n_indices[ind],
                                  translation[ind][point], DEFAULT_CHUNK_SIZE );
        }

        ADD_ELEMENT_TO_ARRAY( out[ind]->end_indices, out[ind]->n_items,
                              n_indices[ind], DEFAULT_CHUNK_SIZE );
    }

    FREE( included );

    for_less( p, 0, polygons->n_points )
    {
        translation1[p] = -1;
        translation2[p] = -1;
    }

    for_less( p, 0, polygons->n_points )
    {
        if( translation[0][p] >= 0 )
            translation1[translation[0][p]] = p;

        if( translation[1][p] >= 0 )
            translation2[translation[1][p]] = p;
    }

    FREE( translation[0] );
    FREE( translation[1] );

    return( n_included );
}
