#include <internal_volume_io.h>
#include <bicpl.h>

private  void   manifold_polygons(
    polygons_struct    *polygons,
    int                start_poly,
    polygons_struct    *out );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  output.obj \n\
\n\
     Creates a 2d manifold from an arbitrary polyhedron.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, output_filename;
    int              n_objects;
    int              start_poly;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons, out;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 0, &start_poly );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects < 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "File must have a polygons structure.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    check_polygons_neighbours_computed( polygons );

    manifold_polygons( polygons, start_poly, &out );

    delete_polygons( polygons );
    *polygons = out;

/*
    (void) output_graphics_file( output_filename, format, n_objects,
                                 object_list );
*/

    return( 0 );
}

private  void   manifold_polygons(
    polygons_struct    *polygons,
    int                start_poly,
    polygons_struct    *out )
{
    int                point, poly, size, neigh, neigh_size, v, edge, p;
    int                current, n_included;
    BOOLEAN            add;
    Smallest_int       *point_included, *poly_included;
    QUEUE_STRUCT(int)  queue;

    for_less( poly, 0, polygons->n_items )
    {
        if( GET_OBJECT_SIZE( *polygons, poly ) != 3 )
        {
            print_error( "Must be triangulation.\n" );
            return;
        }
    }

    ALLOC( poly_included, polygons->n_items );
    for_less( poly, 0, polygons->n_items )
        poly_included[poly] = FALSE;

    ALLOC( point_included, polygons->n_items );
    for_less( point, 0, polygons->n_points )
        point_included[point] = FALSE;
    
    INITIALIZE_QUEUE( queue );

    poly_included[start_poly] = TRUE;
    size = GET_OBJECT_SIZE( *polygons, start_poly );
    for_less( p, 0, size )
    {
        point_included[polygons->indices[POINT_INDEX(polygons->end_indices,
                                   start_poly,p)]] = TRUE;
    }

    INSERT_IN_QUEUE( queue, start_poly );

    while( !IS_QUEUE_EMPTY(queue) )
    {
        REMOVE_FROM_QUEUE( queue, current );

        size = GET_OBJECT_SIZE( *polygons, current );
        for_less( edge, 0, size )
        {
            neigh = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                                     current,edge)];

            if( poly_included[neigh] )
                continue;

            neigh_size = GET_OBJECT_SIZE( *polygons, neigh );
            n_included = 0;
            for_less( v, 0, neigh_size )
            {
                p = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                  neigh,v)];
                if( point_included[p] )
                    ++n_included;
            }

            if( n_included != 2 && n_included != 3 )
            {
                print( "N included: %d\n", n_included );
                continue;
            }

            if( n_included == 2 )
                add = TRUE;
            else
            {
                n_included = 0;
                for_less( v, 0, neigh_size )
                {
                    if( poly_included[polygons->neighbours[
                               POINT_INDEX(polygons->end_indices,neigh,v)]] )
                        ++n_included;
                }

                if( n_included == 0 )
                {
                    print( "N poly included: %d\n", n_included );
                    continue;
                }

                if( n_included == 1 )
                    add = FALSE;
                else
                    add = TRUE;
            }

            if( add )
            {
                poly_included[neigh] = TRUE;
                for_less( p, 0, neigh_size )
                {
                    point_included[polygons->indices[POINT_INDEX(
                                     polygons->end_indices,neigh,p)]] = TRUE;
                }

                INSERT_IN_QUEUE( queue, neigh );
            }
        }
    }

    DELETE_QUEUE( queue );

    n_included = 0;
    for_less( poly, 0, polygons->n_items )
    {
        if( poly_included[poly] )
            ++n_included;
    }

    print( "Polygons labeled: %d / %d\n", n_included, polygons->n_items ); 
}
