#include  <mni.h>

private  void    create_transform_from_3_points(
    Point      points[],
    Transform  *transform );

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_transform_filename;
    int                  i, n_objects, n_points;
    object_struct        **object_list;
    marker_struct        *marker;
    Transform            transform;
    Point                points[3];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_transform_filename ) )
    {
        print( "Usage:  %s  tag_file  output_transform_file\n", argv[0] );
        return( 1 );
    }

    if( input_objects_any_format( (Volume) NULL, input_filename,
                                  WHITE, 1.0, BOX_MARKER,
                                  &n_objects, &object_list ) != OK )
        return( 1 );

    n_points = 0;
    for_less( i, 0, n_objects )
    {
        if( get_object_type( object_list[i] ) == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            if( n_points < 3 )
                points[n_points] = marker->position;
            ++n_points;
        }
    }

    delete_object_list( n_objects, object_list );

    if( n_points != 3 )
    {
        print( "Need exactly 3 points.\n" );
        return( 1 );
    }

    create_transform_from_3_points( points, &transform );

    (void) write_transform_file( output_transform_filename, "", &transform );

    return( 0 );
}

private  void    create_transform_from_3_points(
    Point      points[],
    Transform  *transform )
{
    int      i, axis, a1, a2;
    Real     x, y, z, xt, yt, zt;
    Vector   v1, v2, cross, axes[N_DIMENSIONS], diff;
    Point    origin, pre_centroid, post_centroid;

    SUB_POINTS( v1, points[1], points[0] );
    SUB_POINTS( v2, points[2], points[0] );
    CROSS_VECTORS( cross, v1, v2 );
    NORMALIZE_VECTOR( cross, cross );

    axis = X;
    for_less( i, 1, N_DIMENSIONS )
    {
        if( ABS(Vector_coord(cross,i)) > ABS(Vector_coord(cross,axis)) )
            axis = i;
    }

    if( Vector_coord( cross, axis ) < 0.0 )
        SCALE_VECTOR( cross, cross, -1.0 );

    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    axes[axis] = cross;
    fill_Vector( axes[a1], 0.0, 0.0, 0.0 );
    Vector_coord( axes[a1], a1 ) = 1.0;
    CROSS_VECTORS( axes[a2], cross, axes[a1] );
    CROSS_VECTORS( axes[a1], axes[a2], cross );

    NORMALIZE_VECTOR( axes[a1], axes[a1] );
    NORMALIZE_VECTOR( axes[a2], axes[a2] );

    fill_Vector( origin, 0.0, 0.0, 0.0 );

    make_change_from_bases_transform( &origin, &axes[X], &axes[Y], &axes[Z],
                                      transform );

    get_points_centroid( 3, points, &pre_centroid );

    x = 0.0;
    y = 0.0;
    z = 0.0;

    for_less( i, 0, 3 )
    {
        transform_point( transform, Point_x(points[i]),
                         Point_y(points[i]), Point_z(points[i]), &xt, &yt, &zt);
        x += xt;
        y += yt;
        z += zt;
    }

    Point_coord( pre_centroid, axis ) = 0.0;
    fill_Vector( post_centroid, x / 3.0, y / 3.0, z / 3.0 );

    SUB_POINTS( diff, pre_centroid, post_centroid );

    CONVERT_VECTOR_TO_POINT( origin, diff );

    set_transform_origin( transform, &origin );
}
