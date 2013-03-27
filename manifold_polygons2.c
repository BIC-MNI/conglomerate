#include <volume_io.h>
#include <bicpl.h>

private  void   manifold_polygons(
    polygons_struct    *polygons,
    int                start_poly,
    int                max_polygons,
    VIO_BOOL            manifold_required,
    polygons_struct    *out );

private  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input.obj  output.obj [x y z] [max_poly]\n\
\n\
     Creates a 2d manifold from an arbitrary polyhedron.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR           input_filename, output_filename, dummy;
    int              n_objects, max_polygons;
    int              start_poly, poly;
    VIO_Real             x, y, z, dist, closest_dist;
    VIO_Point            start, point;
    VIO_BOOL          start_specified, manifold_required;
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

    if( get_real_argument( 0.0, &x ) &&
        get_real_argument( 0.0, &y ) &&
        get_real_argument( 0.0, &z ) )
    {
        start_specified = TRUE;
        fill_Point( start, x, y, z );
    }
    else
        start_specified = FALSE;

    (void) get_int_argument( -1, &max_polygons );
    manifold_required = !get_string_argument( NULL, &dummy );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK || n_objects < 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "File must have a polygons structure.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    check_polygons_neighbours_computed( polygons );

    if( start_specified )
    {
        start_poly = -1;
        closest_dist = 0.0;
        for_less( poly, 0, polygons->n_items )
        {
            dist = get_point_object_distance_sq( &start, object_list[0],
                                                 poly, &point );

            if( start_poly < 0 || dist < closest_dist )
            {
                start_poly = poly;
                closest_dist = dist;
            }
        }
    }
    else
    {
        start_poly = 0;
    }

    manifold_polygons( polygons, start_poly, max_polygons, manifold_required,
                       &out );

    print( "Extracted %d polygons from %d\n", out.n_items, polygons->n_items );

    delete_polygons( polygons );
    *polygons = out;

    (void) output_graphics_file( output_filename, format, n_objects,
                                 object_list );

    return( 0 );
}

private  void   assign_distances(
    polygons_struct    *polygons,
    int                poly_dist[],
    int                start_poly )
{
    int                         poly, size, neigh, edge, current, current_dist;
    VIO_Real                        dist;
    PRIORITY_QUEUE_STRUCT(int)  queue;

    for_less( poly, 0, polygons->n_items )
        poly_dist[poly] = -1;

    INITIALIZE_PRIORITY_QUEUE( queue );

    poly_dist[start_poly] = 0;

    INSERT_IN_PRIORITY_QUEUE( queue, start_poly, 0.0 );

    while( !IS_PRIORITY_QUEUE_EMPTY(queue) )
    {
        REMOVE_FROM_PRIORITY_QUEUE( queue, current, dist );
        current_dist = poly_dist[current];

        size = GET_OBJECT_SIZE( *polygons, current );
        for_less( edge, 0, size )
        {
            neigh = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                                     current,edge)];

            if( neigh >= 0 &&
                (poly_dist[neigh] < 0 || poly_dist[neigh] > current_dist + 1) )
            {
                poly_dist[neigh] = current_dist + 1;
                INSERT_IN_PRIORITY_QUEUE( queue, neigh,
                                          (VIO_Real) -poly_dist[neigh] );
            }
        }
    }

    DELETE_PRIORITY_QUEUE( queue );
}

private  void   manifold_polygons(
    polygons_struct    *polygons,
    int                start_poly,
    int                max_polygons,
    VIO_BOOL            manifold_required,
    polygons_struct    *out )
{
    int                point, poly, size, neigh, neigh_size, v, edge, p, i, j;
    int                best;
    int                current, n_points_included, n_polys_included;
    int                ind, *new_point_ids, n;
    int                n_done, *sorted, tmp;
    int                *poly_dist;
    VIO_BOOL            add, changed;
    Smallest_int       *point_included, *poly_included;

    for_less( poly, 0, polygons->n_items )
    {
        if( GET_OBJECT_SIZE( *polygons, poly ) != 3 )
        {
            print_error( "Must be triangulation.\n" );
            return;
        }
    }

    ALLOC( poly_dist, polygons->n_items );
    assign_distances( polygons, poly_dist, start_poly );

    ALLOC( sorted, polygons->n_items );
    for_less( i, 0, polygons->n_items )
        sorted[i] = i;

    for_less( i, 0, polygons->n_items-1 )
    {
        best = i;
        for_less( j, i+1, polygons->n_items )
        {
            if( poly_dist[sorted[j]] < poly_dist[sorted[best]] )
                best = j;
        }

        tmp = sorted[i];
        sorted[i] = sorted[best];
        sorted[best] = tmp;
    }

    FREE( poly_dist );

    ALLOC( poly_included, polygons->n_items );
    for_less( poly, 0, polygons->n_items )
        poly_included[poly] = FALSE;

    ALLOC( point_included, polygons->n_items );
    for_less( point, 0, polygons->n_points )
        point_included[point] = FALSE;
    
    poly_included[start_poly] = TRUE;
    size = GET_OBJECT_SIZE( *polygons, start_poly );
    for_less( p, 0, size )
    {
        point_included[polygons->indices[POINT_INDEX(polygons->end_indices,
                                   start_poly,p)]] = TRUE;
    }

    n_done = 1;
    changed = TRUE;

    while( changed && (max_polygons <= 0 || n_done < max_polygons) )
    {
        changed = FALSE;
        for_less( i, 0, polygons->n_items )
        {
            current = sorted[i];
            size = GET_OBJECT_SIZE( *polygons, current );
            for_less( edge, 0, size )
            {
                neigh = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                                         current,edge)];

                if( neigh < 0 || poly_included[neigh] )
                    continue;

                neigh_size = GET_OBJECT_SIZE( *polygons, neigh );

                if( !manifold_required )
                    add = TRUE;
                else
                {
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
                }

                if( add && (max_polygons <= 0 || n_done < max_polygons) )
                {
                    poly_included[neigh] = TRUE;
                    for_less( p, 0, neigh_size )
                    {
                        point_included[polygons->indices[POINT_INDEX(
                                         polygons->end_indices,neigh,p)]] = TRUE;
                    }

                    ++n_done;
                    changed = TRUE;
                }

                if( changed )
                    break;
            }
            if( changed )
                break;
        }
    }

    FREE( sorted );

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
        else if( max_polygons <= 0 && !manifold_required )
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
