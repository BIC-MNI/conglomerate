#include <internal_volume_io.h>
#include <bicpl.h>

private  int   separate_polygons(
    polygons_struct    *polygons,
    int                desired_index,
    object_struct      **out[] );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  output_prefix [which] \n\
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
    int              i, desired_index;
    File_formats     format;
    object_struct    **object_list, **out;
    polygons_struct  *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_prefix ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -1, &desired_index );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects < 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "File must have a polygons structure.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    check_polygons_neighbours_computed( polygons );

    n_out = separate_polygons( polygons, desired_index, &out );

    for_less( i, 0, n_out )
    {
        if( n_out == 1 )
            (void) sprintf( out_filename, "%s", output_prefix );
        else
            (void) sprintf( out_filename, "%s_%d.obj", output_prefix, i+1 );

        (void) output_graphics_file( out_filename, format, 1, &out[i] );
    }

    return( 0 );
}

private  int   make_connected_components(
    polygons_struct    *polygons,
    Smallest_int       polygon_classes[],
    int                n_in_class[] )
{
    int                poly, current_poly, edge, size;
    int                neigh;
    int                n_components;
    Smallest_int       not_done;
    QUEUE_STRUCT(int)  queue;

    n_components = 0;

    not_done = (Smallest_int) 255;

    for_less( poly, 0, polygons->n_items )
        polygon_classes[poly] = not_done;

    for_less( poly, 0, polygons->n_items )
    {
        if( polygon_classes[poly] != not_done )
            continue;

        if( n_components == 255 )
        {
            ++n_components;
            break;
        }

        INITIALIZE_QUEUE( queue );
        INSERT_IN_QUEUE( queue, poly );
        polygon_classes[poly] = (Smallest_int) n_components;
        n_in_class[n_components] = 1;

        while( !IS_QUEUE_EMPTY(queue) )
        {
            REMOVE_FROM_QUEUE( queue, current_poly );
            size = GET_OBJECT_SIZE( *polygons, current_poly );

            for_less( edge, 0, size )
            {
                neigh = polygons->neighbours[
                    POINT_INDEX(polygons->end_indices,current_poly,edge)];
                if( neigh >= 0 &&
                    polygon_classes[neigh] == not_done )
                {
                    polygon_classes[neigh] = (Smallest_int) n_components;
                    ++n_in_class[n_components];
                    INSERT_IN_QUEUE( queue, neigh );
                }
            }
        }

        DELETE_QUEUE( queue );

        ++n_components;
    }

    return( n_components );
}

private  int   separate_polygons(
    polygons_struct    *polygons,
    int                desired_index,
    object_struct      **out[] )
{
    int                point, ind, p_ind, poly, vertex, size, i, j, tmp;
    int                point_index, *new_point_ids, n_objects, comp, c;
    int                biggest;
    Smallest_int       *poly_classes;
    int                n_components, *n_in_class, *ordered;
    polygons_struct    *new_poly;

    ALLOC( poly_classes, polygons->n_items );
    ALLOC( n_in_class, 256 );
    ALLOC( ordered, 256 );

    n_components = make_connected_components( polygons, poly_classes,
                                              n_in_class );

    for_less( i, 0, n_components )
        ordered[i] = i;

    for_less( i, 0, n_components-1 )
    {
        biggest = i;
        for_less( j, i+1, n_components )
        {
            if( n_in_class[ordered[j]] > n_in_class[ordered[biggest]] )
                biggest = j;
        }

        tmp = ordered[i];
        ordered[i] = ordered[biggest];
        ordered[biggest] = tmp;
    }

    ALLOC( new_point_ids, polygons->n_points );

    n_objects = 0;

    for_less( c, 0, n_components )
    {
        if( desired_index >= 0 && c != desired_index )
            continue;

        comp = ordered[c];

        for_less( point, 0, polygons->n_points )
            new_point_ids[point] = -1;

        SET_ARRAY_SIZE( *out, n_objects, n_objects+1,
                        DEFAULT_CHUNK_SIZE);
        (*out)[n_objects] = create_object( POLYGONS );
        new_poly = get_polygons_ptr( (*out)[n_objects] );
        ++n_objects;
        initialize_polygons( new_poly, WHITE, NULL );
        if( desired_index >= 0 )
        {
            new_poly->points = polygons->points;
            new_poly->normals = polygons->normals;
            new_poly->indices = polygons->indices;
            new_poly->n_items = 0;

            for_less( poly, 0, polygons->n_items )
            {
                if( poly_classes[poly] != (Smallest_int) comp )
                    continue;
                size = GET_OBJECT_SIZE( *polygons, poly );
                ++new_poly->n_items;
                for_less( vertex, 0, size )
                {
                    point_index = polygons->indices[
                              POINT_INDEX(polygons->end_indices,poly,vertex)];
                    if( new_point_ids[point_index] < 0 )
                        new_point_ids[point_index] = 0;
                }
            }

            ALLOC( new_poly->end_indices, new_poly->n_items );

            ind = 0;
            for_less( point, 0, polygons->n_points )
            {
                if( new_point_ids[point] >= 0 )
                {
                    new_point_ids[point] = ind;
                    new_poly->points[ind] = new_poly->points[point];
                    new_poly->normals[ind] = new_poly->normals[point];
                    ++ind;
                }
            }

            new_poly->n_points = ind;

            p_ind = 0;
            ind = 0;
            for_less( poly, 0, polygons->n_items )
            {
                if( poly_classes[poly] != (Smallest_int) comp )
                    continue;

                size = GET_OBJECT_SIZE( *polygons, poly );

                for_less( vertex, 0, size )
                {
                    point_index = polygons->indices[
                         POINT_INDEX(polygons->end_indices,poly,vertex)];
                    new_poly->indices[ind] = new_point_ids[point_index];
                    ++ind;
                }

                new_poly->end_indices[p_ind] = ind;
                ++p_ind;
            }
        }
        else
        {
            ind = 0;
            for_less( poly, 0, polygons->n_items )
            {
                if( poly_classes[poly] != (Smallest_int) comp )
                    continue;

                size = GET_OBJECT_SIZE( *polygons, poly );
                for_less( vertex, 0, size )
                {
                    point_index = polygons->indices[
                              POINT_INDEX(polygons->end_indices,poly,vertex)];

                    if( new_point_ids[point_index] < 0 )
                    {
                        new_point_ids[point_index] = new_poly->n_points;
                        ADD_ELEMENT_TO_ARRAY( new_poly->points,
                                              new_poly->n_points,
                                              polygons->points[point_index],
                                              DEFAULT_CHUNK_SIZE );
                        --new_poly->n_points;
                        ADD_ELEMENT_TO_ARRAY( new_poly->normals,
                                              new_poly->n_points,
                                              polygons->normals[point_index],
                                              DEFAULT_CHUNK_SIZE );
                    }

                    ADD_ELEMENT_TO_ARRAY( new_poly->indices, ind,
                                          new_point_ids[point_index],
                                          DEFAULT_CHUNK_SIZE );
                }

                ADD_ELEMENT_TO_ARRAY( new_poly->end_indices, new_poly->n_items,
                                      ind, DEFAULT_CHUNK_SIZE );
            }

            REALLOC( new_poly->points, new_poly->n_points );
            REALLOC( new_poly->normals, new_poly->n_points );
            REALLOC( new_poly->end_indices, new_poly->n_items );
            REALLOC( new_poly->indices, ind );
        }
    }

    FREE( poly_classes );
    FREE( new_point_ids );
    FREE( n_in_class );
    FREE( ordered );

    return( n_objects );
}
