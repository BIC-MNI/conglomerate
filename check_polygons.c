#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  test_polygon_integrity(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[] );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename;
    int                  n_objects;
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

    return( 0 );
}

private  void  test_polygon_integrity(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[] )
{
    int    poly, size, p, n, n1, v, v1, n_points;

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

    for_less( p, 0, n_points )
    {
        for_less( n, 0, n_neighbours[p]-1 )
        for_less( n1, n+1, n_neighbours[p] )
            if( neighbours[p][n] == neighbours[p][n1] )
                print( "Point %d: %d\n", p, neighbours[p][n] );
    }
}
