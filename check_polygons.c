#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  test_polygon_integrity(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[] );

private  int  count_edges(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[] );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename;
    int                  n_objects, n_edges;
    int                  *n_neighbours, **neighbours;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) )
    {
        print_error( "Usage: %s  input.obj\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    create_polygon_point_neighbours( polygons, TRUE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    test_polygon_integrity( polygons, n_neighbours, neighbours );

    n_edges = count_edges( polygons, n_neighbours, neighbours );

    delete_polygon_point_neighbours( polygons, n_neighbours,
                                     neighbours, NULL, NULL );

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    n_edges = count_edges( polygons, n_neighbours, neighbours );

    print( "%d + %d - %d = %d\n", polygons->n_items, polygons->n_points,
                                  n_edges,
               polygons->n_items + polygons->n_points - n_edges );

    return( 0 );
}

private  void  test_closed_surface(
    polygons_struct  *polygons )
{
    int           p, poly, *n_neighbours, **neighbours, edge;
    int           p1, p2, t1, t2, total_neighbours, vertex, n_found;
    int           size, size2, poly2, edge2, point_index, n, edge3;

    ALLOC( n_neighbours, polygons->n_points );
    for_less( p, 0, polygons->n_points )
        n_neighbours[p] = 0;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( vertex, 0, size )
        {
            point_index = polygons->indices[POINT_INDEX(polygons->end_indices,
                                            poly,vertex)];
            ++n_neighbours[point_index];
        }
    }

    total_neighbours = 0;
    for_less( p, 0, polygons->n_points )
        total_neighbours += n_neighbours[p];

    ALLOC( neighbours, polygons->n_points );
    ALLOC( neighbours[0], total_neighbours );
    total_neighbours = 0;
    for_less( p, 1, polygons->n_points )
        neighbours[p] = &neighbours[p-1][n_neighbours[p-1]];
    for_less( p, 0, polygons->n_points )
        n_neighbours[p] = 0;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( vertex, 0, size )
        {
            point_index = polygons->indices[POINT_INDEX(polygons->end_indices,
                                            poly,vertex)];
            neighbours[point_index][n_neighbours[point_index]] = poly;
            ++n_neighbours[point_index];
        }
    }

    if( getenv("POINT") != NULL && sscanf( getenv("POINT"), "%d", &p2 ) == 1 )
    {
        for_less( poly, 0, polygons->n_items )
        {
            size = GET_OBJECT_SIZE( *polygons, poly );
            for_less( edge, 0, size )
            {
                p1 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                   poly,edge)];

                if( p1 == p2 )
                    break;
            }
            if( edge < size )
            {
                print( "Poly %d: ", poly );
                for_less( edge, 0, size )
                {
                    p1 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                       poly,edge)];
                    print( " %d", p1 );
                }
                print( "\n" );
            }
        }
    }

    /*----------------------------------------*/

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( edge, 0, size )
        {
            p1 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                               poly,edge)];
            p2 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                               poly,(edge+1)%size)];

            n_found = 0;
            for_less( n, 0, n_neighbours[p1] )
            {
                poly2 = neighbours[p1][n];
                if( poly2 == poly )
                    continue;

                size2 = GET_OBJECT_SIZE( *polygons, poly2 );
                for_less( edge2, 0, size2 )
                {
                    t1 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                       poly2,edge2)];
                    if( t1 == p1 )
                        break;
                }
                for_less( edge3, 0, size2 )
                {
                    t2 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                       poly2,edge3)];
                    if( t2 == p2 )
                        break;
                }

                if( edge2 < size2 && edge3 < size2 )
                {
                    if( (edge3 + 1) % size2 == edge2 )
                        ++n_found;
                    else
                        print_error( "Edge facing the wrong way\n" );
                }
            }

            if( n_found > 1 )
                print( "n_found = %d\n", n_found );
        }
    }

    FREE( neighbours[0] );
    FREE( neighbours );
    FREE( n_neighbours );
}

private  void  test_polygon_integrity(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[] )
{
    int    poly, size, p, n, n1, v, v1, n_points, p1, p2, p3;
    Vector v12, v13, cross;

    n_points = polygons->n_points;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( v, 0, size-1 )
        for_less( v1, v+1, size )
            if( polygons->indices[POINT_INDEX(polygons->end_indices,poly,v)] ==
                polygons->indices[POINT_INDEX(polygons->end_indices,poly,v1)] )
                print( "Poly %d, vertex %d, %d = %d\n",
                       poly, v, v1, polygons->indices[POINT_INDEX(polygons->end_indices,poly,v)] );
    }

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        if( size != 3 )
            continue;

        p1 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,0)];
        p2 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,1)];
        p3 = polygons->indices[POINT_INDEX(polygons->end_indices,poly,2)];

        SUB_POINTS( v12, polygons->points[p2], polygons->points[p1] );
        SUB_POINTS( v13, polygons->points[p3], polygons->points[p1] );
        CROSS_VECTORS( cross, v12, v13 );
        if( null_Vector( &cross ) )
        {
            print( "Triangle %d: %d %d %d has zero area.\n", poly, p1, p2, p3 );
        }
    }

    for_less( p, 0, n_points )
    {
        for_less( n, 0, n_neighbours[p]-1 )
        for_less( n1, n+1, n_neighbours[p] )
            if( neighbours[p][n] == neighbours[p][n1] )
                print( "Point %d: %d\n", p, neighbours[p][n] );
    }

    test_closed_surface( polygons );

#ifdef TEST_FOR_CLOSE_POINTS
    for_less( p, 0, n_points-1 )
    {
        for_less( p1, p+1, n_points )
        {
            if( sq_distance_between_points( &polygons->points[p],
                                            &polygons->points[p1] ) < 1e-8 )
            {
                print( "Points close to equal %d %d\n", p, p1 );
                print( "               %g %g %g\n",
                       RPoint_x(polygons->points[p]),
                       RPoint_y(polygons->points[p]),
                       RPoint_z(polygons->points[p]) );
                print( "               %g %g %g\n",
                       RPoint_x(polygons->points[p1]),
                       RPoint_y(polygons->points[p1]),
                       RPoint_z(polygons->points[p1]) );
            }
        }
    }
#endif
}

private  int  count_edges(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[] )
{
    int    point, n, nn, n_edges, n_duplicate_edges, neigh, n_points;

    n_points = polygons->n_points;

    n_edges = 0;
    n_duplicate_edges = 0;
    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            for_less( nn, n+1, n_neighbours[point] )
            {
                if( neighbours[point][nn] == neigh )
                    break;
            }
            if( nn < n_neighbours[point] )
                ++n_duplicate_edges;
            else if( point < neigh )
                ++n_edges;
        }
    }

    if( n_duplicate_edges > 0 )
        print( "N duplicate edges: %d\n", n_duplicate_edges );

    return( n_edges );
}
