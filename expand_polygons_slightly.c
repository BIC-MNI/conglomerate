#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  expand_polygons(
    polygons_struct  *polygons,
    Real             distance );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename;
    Real                 dist;
    int                  n_objects;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_real_argument( 0.0, &dist ) )
    {
        print_error( "Usage: %s  input.obj  output.obj dist\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    expand_polygons( get_polygons_ptr(object_list[0]), dist );

    (void) output_graphics_file( dest_filename, format, n_objects,
                                 object_list );

    return( 0 );
}

private  void  expand_polygons(
    polygons_struct  *polygons,
    Real             distance )
{
    int           *n_neighbours, **neighbours;
    int           p, poly, edge, n_points, i;
    int           p1, p2, t1, t2, total_neighbours, vertex, n_found;
    int           n, point, n_in_plane, new_n_in_plane;
    Real          plane_constant;
    Point         *neigh_points, *plane_points, *temp_plane_points;
    Point         centroid, *points, plane_origin;
    Vector        normal, to_point, plane_normal, clip_normal;
    Vector        hor, vert;

    n_points = polygons->n_points;
    points = polygons->points;

    create_polygon_point_neighbours( polygons, TRUE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    ALLOC( neigh_points, n_points );
    ALLOC( plane_points, n_points );
    ALLOC( temp_plane_points, n_points );

    for_less( point, 0, polygons->n_points )
    {
        for_less( n, 0, n_neighbours[point] )
            neigh_points[n] = points[neighbours[point][n]];

        get_points_centroid( n_neighbours[point], neigh_points, &centroid );
        find_polygon_normal( n_neighbours[point], neigh_points, &normal );

        SUB_POINTS( to_point, points[point], centroid );
        if( DOT_VECTORS( to_point, normal ) < 0.0 )
        {
            SCALE_VECTOR( to_point, to_point, -1.0 );
        }
        NORMALIZE_VECTOR( to_point, to_point );

        GET_POINT_ON_RAY( plane_origin, points[point], to_point, 1.0 );
        plane_normal = to_point;

        create_two_orthogonal_vectors( &plane_normal, &hor, &vert );
        SCALE_VECTOR( hor, hor, 1.0e6 );
        SCALE_VECTOR( vert, vert, 1.0e6 );

        n_in_plane = 4;
        
        ADD_POINT_VECTOR( plane_points[0], plane_origin, hor );
        ADD_POINT_VECTOR( plane_points[1], plane_origin, vert );
        SUB_POINT_VECTOR( plane_points[2], plane_origin, hor );
        SUB_POINT_VECTOR( plane_points[3], plane_origin, vert );

        for_less( n, 0, n_neighbours[point] )
        {
            temp_plane_points[0] = points[point];
            temp_plane_points[1] = points[neighbours[point][n]];
            temp_plane_points[2] = points[neighbours[point]
                                           [(n+1)%n_neighbours[point]]];
            find_polygon_normal( 3, temp_plane_points, &clip_normal );
            plane_constant = -DOT_POINT_VECTOR( points[point], clip_normal );

            new_n_in_plane = clip_polygon_against_plane(
                                  n_in_plane, plane_points,
                                  plane_constant, &clip_normal,
                                  temp_plane_points );

            for_less( i, 0, new_n_in_plane )
                plane_points[i] = temp_plane_points[i];
            n_in_plane = new_n_in_plane;

            if( n_in_plane < 3 )
                handle_internal_error( "dang" );
        }
    }

    FREE( neigh_points );
    FREE( plane_points );
    FREE( temp_plane_points );
}
