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

    (void) output_graphics_file( output_filename, format, n_objects,
                                 object_list );

    return( 0 );
}

private  void   manifold_polygons(
    polygons_struct    *polygons,
    int                start_poly,
    polygons_struct    *out )
{
    int                point, poly, size, neigh, neigh_size, v, edge, p;
    int                current, n_points_included, n_polys_included;
    int                ind, *new_point_ids, n;
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

            if( neigh < 0 || poly_included[neigh] )
                continue;

            neigh_size = GET_OBJECT_SIZE( *polygons, neigh );
            n_points_included = 0;
            for_less( v, 0, neigh_size )
            {
                p = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                  neigh,v)];
                if( point_included[p] )
                    ++n_points_included;
            }

            if( n_points_included != 2 && n_points_included != 3 )
            {
                print( "N included: %d\n", n_points_included );
                continue;
            }

            if( n_points_included == 2 )
                add = TRUE;
            else
            {
                n_polys_included = 0;
                for_less( v, 0, neigh_size )
                {
                    n = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                                         neigh,v)];
                    if( n < 0 || poly_included[n] )
                        ++n_polys_included;
                }

                if( n_polys_included == 0 )
                {
                    print( "N poly included: %d\n", n_polys_included );
                    continue;
                }

                if( n_polys_included == 1 )
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

    n_points_included = 0;
    for_less( p, 0, polygons->n_points )
    {
        if( point_included[p] )
            ++n_points_included;
    }

    n_polys_included = 0;
    for_less( poly, 0, polygons->n_items )
    {
        if( poly_included[poly] )
            ++n_polys_included;
        else
        {
            int  n;
            size = GET_OBJECT_SIZE( *polygons, poly );
            n = 0;
            for_less( v, 0, neigh_size )
            {
                n = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                                     poly,v)];
                if( n < 0 || poly_included[n] )
                    ++n;
            }

            if( n == 2 || n == 3 )
                handle_internal_error( "holey" );
        }
    }

    print( "Polygons labeled: %d / %d\n", n_polys_included, polygons->n_items ); 
    initialize_polygons( out, WHITE, NULL );

    out->n_points = n_points_included;
    ALLOC( out->points, n_points_included );
    ALLOC( out->normals, n_points_included );
    ALLOC( new_point_ids, polygons->n_points );
    ind = 0;
    for_less( p, 0, polygons->n_points )
    {
        if( point_included[p] )
        {
            new_point_ids[p] = ind;
            out->points[ind] = polygons->points[p];
            out->normals[ind] = polygons->normals[p];
            ++ind;
        }
        else
            new_point_ids[p] = -1;
    }

    out->n_items = n_polys_included;
    ALLOC( out->end_indices, n_polys_included );
    for_less( poly, 0, n_polys_included )
        out->end_indices[poly] = 3 * (poly+1);
    ALLOC( out->indices, 3 * n_polys_included );

    ind = 0;
    for_less( poly, 0, polygons->n_items )
    {
        if( !poly_included[poly] )
            continue;

        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( v, 0, size )
        {
            p = polygons->indices[POINT_INDEX(polygons->end_indices,poly,v)];

            out->indices[ind] = new_point_ids[p];
            ++ind;
        }
    }

    if( ind != 3 * n_polys_included )
        handle_internal_error( "ind" );

    FREE( new_point_ids );
}
