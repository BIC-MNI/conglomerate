#include <internal_volume_io.h>
#include <bicpl.h>

private  int   separate_polygons(
    polygons_struct    *polygons,
    object_struct      **out[] );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  output_prefix \n\
\n\
     Separates polygons into its disjoint parts.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, output_prefix;
    char             out_filename[EXTREMELY_LARGE_STRING_SIZE];
    int              n_objects, n_out;
    int              i, j, biggest;
    File_formats     format;
    object_struct    **object_list, **out, *tmp;
    polygons_struct  *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_prefix ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects < 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "File must have a polygons structure.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    check_polygons_neighbours_computed( polygons );

    n_out = separate_polygons( polygons, &out );

    for_less( i, 0, n_out-1 )
    {
        biggest = i;
        for_less( j, i+1, n_out )
        {
            if( get_polygons_ptr(out[j])->n_items >
                get_polygons_ptr(out[biggest])->n_items )
                biggest = j;
        }

        tmp = out[i];
        out[i] = out[biggest];
        out[biggest] = tmp;
    }

    for_less( i, 0, n_out )
    {
        (void) sprintf( out_filename, "%s_%d.obj", output_prefix, i+1 );

        (void) output_graphics_file( out_filename, format, 1, &out[i] );
    }

    return( 0 );
}

private  int   separate_polygons(
    polygons_struct    *polygons,
    object_struct      **out[] )
{
    int                point, ind, poly, current_poly, edge, vertex, size;
    int                point_index, *new_point_ids, neigh;
    Smallest_int       *poly_done;
    int                n_components;
    QUEUE_STRUCT(int)  queue;
    polygons_struct    *new_poly;

    n_components = 0;

    ALLOC( poly_done, polygons->n_items );
    for_less( poly, 0, polygons->n_items )
        poly_done[poly] = FALSE;
    ALLOC( new_point_ids, polygons->n_points );

    for_less( poly, 0, polygons->n_items )
    {
        if( poly_done[poly] )
            continue;

        for_less( point, 0, polygons->n_points )
            new_point_ids[point] = -1;

        INITIALIZE_QUEUE( queue );
        INSERT_IN_QUEUE( queue, poly );
        poly_done[poly] = TRUE;
        SET_ARRAY_SIZE( *out, n_components, n_components+1, DEFAULT_CHUNK_SIZE);
        (*out)[n_components] = create_object( POLYGONS );
        new_poly = get_polygons_ptr( (*out)[n_components] );
        ++n_components;
        initialize_polygons( new_poly, WHITE, NULL );
        ind = 0;

        while( !IS_QUEUE_EMPTY(queue) )
        {
            REMOVE_FROM_QUEUE( queue, current_poly );
            size = GET_OBJECT_SIZE( *polygons, current_poly );

            for_less( vertex, 0, size )
            {
                point_index = polygons->indices[
                   POINT_INDEX(polygons->end_indices,current_poly,vertex)];
                if( new_point_ids[point_index] < 0 )
                {
                    new_point_ids[point_index] = new_poly->n_points;
                    ADD_ELEMENT_TO_ARRAY( new_poly->points, new_poly->n_points,
                                          polygons->points[point_index],
                                          DEFAULT_CHUNK_SIZE );
                    --new_poly->n_points;
                    ADD_ELEMENT_TO_ARRAY( new_poly->normals, new_poly->n_points,
                                          polygons->normals[point_index],
                                          DEFAULT_CHUNK_SIZE );
                }

                ADD_ELEMENT_TO_ARRAY( new_poly->indices, ind,
                                      new_point_ids[point_index],
                                      DEFAULT_CHUNK_SIZE );
            }

            ADD_ELEMENT_TO_ARRAY( new_poly->end_indices, new_poly->n_items, ind,
                                  DEFAULT_CHUNK_SIZE );

            for_less( edge, 0, size )
            {
                neigh = polygons->neighbours[
                    POINT_INDEX(polygons->end_indices,current_poly,edge)];
                if( neigh >= 0 && !poly_done[neigh] )
                {
                    poly_done[neigh] = TRUE;
                    INSERT_IN_QUEUE( queue, neigh );
                }
            }
        }

        DELETE_QUEUE( queue );
    }

    FREE( poly_done );
    FREE( new_point_ids );

    return( n_components );
}
