#include  <internal_volume_io.h>
#include  <special_geometry.h>

#define  X_FACTOR  5.0
#define  Y_FACTOR  20.0

private  void  get_voxel_corners(
    Volume    volume,
    Real      x,
    Real      y,
    Real      z,
    Point     points[] );
private  void    find_plane(
    Point      points[],
    Vector     *normal );
private  void  create_plane_from_points(
    Point             points[],
    Real              dist,
    polygons_struct   *polygons );

int  main(
    int   argc,
    char  *argv[] )
{
    BOOLEAN              create_plane, volume_desired;
    char                 *three_tags_filename, *input_tags_filename;
    char                 *output_tags_filename, *plane_filename;
    char                 *volume_filename;
    Real                 min_dist, max_dist, dist;
    int                  i, n_objects;
    Point                points[3], origin, point;
    Vector               normal;
    Vector               left_normal, right_normal;
    Real                 left_constant, right_constant;
    object_struct        *object, **object_list;
    int                  n_volumes, n_tag_points, *structure_ids, *patient_ids;
    Real                 **tags1, **tags2, *weights, volume;
    char                 **labels;
    int                  n_new_tags, *new_structure_ids, *new_patient_ids;
    Real                 **new_tags1, **new_tags2, *new_weights;
    char                 **new_labels;
    polygons_struct      voxel, tmp, clipped;
    Volume               vol;
    volume_input_struct  volume_input;


    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &three_tags_filename ) ||
        !get_string_argument( "", &input_tags_filename ) ||
        !get_string_argument( "", &output_tags_filename ) ||
        !get_real_argument( 0.0, &min_dist ) ||
        !get_real_argument( 0.0, &max_dist ) )
    {
        print( "Usage:  %s  3_tags_file  input.tag  output.tag  -distance +distance [mnc_file_if_you_want_volume]   [plane_filename]\n", argv[0] );
        return( 1 );
    }

    volume_desired = get_string_argument( "", &volume_filename );
    create_plane = get_string_argument( "", &plane_filename );

    if( min_dist > max_dist )
    {
        Real  tmp;

        tmp = min_dist;
        min_dist = max_dist;
        max_dist = tmp;
    }

    if( input_tag_file( three_tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, &weights, &structure_ids,
                        &patient_ids, &labels ) != OK )
        return( 1 );

    if( n_tag_points != 3 )
    {
        print( "Usage:  %s  3_tags_file  input.tag  output.tag\n", argv[0] );
        print( "First file must contain exactly 3 tags.\n" );
        return( 1 );
    }

    for_less( i, 0, 3 )
        fill_Point( points[i], tags1[i][X], tags1[i][Y], tags1[i][Z] );

    free_tag_points( n_volumes, n_tag_points, tags1, tags2,
                     weights, structure_ids, patient_ids, labels );

    find_plane( points, &normal );
    NORMALIZE_VECTOR( normal, normal );
    origin = points[0];

    print( "Normal is: %g %g %g\n",
           Vector_x(normal), Vector_y(normal), Vector_z(normal) );
    print( "Origin is: %g %g %g\n",
           Point_x(origin), Point_y(origin), Point_z(origin) );

    if( input_tag_file( input_tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, &weights, &structure_ids,
                        &patient_ids, &labels ) != OK )
        return( 1 );

    n_new_tags = 0;

    if( volume_desired )
    {
        if( start_volume_input( volume_filename, 3, XYZ_dimension_names,
                                NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                                TRUE, &vol, (minc_input_options *) NULL,
                                &volume_input ) != OK )
            return( 1 );

        volume = 0.0;
        initialize_polygons( &voxel, WHITE, NULL );
        voxel.n_points = 8;
        ALLOC( voxel.points, 8 );
        ALLOC( voxel.normals, 8 );
        voxel.n_items = 6;
        ALLOC( voxel.end_indices, 6 );
        for_less( i, 0, 6 )
            voxel.end_indices[i] = 4 + 4 * i;
        ALLOC( voxel.indices, 24 );
        i = 0;
        voxel.indices[i++] = 0;
        voxel.indices[i++] = 1;
        voxel.indices[i++] = 3;
        voxel.indices[i++] = 2;

        voxel.indices[i++] = 4;
        voxel.indices[i++] = 6;
        voxel.indices[i++] = 7;
        voxel.indices[i++] = 5;

        voxel.indices[i++] = 0;
        voxel.indices[i++] = 4;
        voxel.indices[i++] = 5;
        voxel.indices[i++] = 1;

        voxel.indices[i++] = 2;
        voxel.indices[i++] = 3;
        voxel.indices[i++] = 7;
        voxel.indices[i++] = 6;

        voxel.indices[i++] = 0;
        voxel.indices[i++] = 2;
        voxel.indices[i++] = 6;
        voxel.indices[i++] = 4;

        voxel.indices[i++] = 1;
        voxel.indices[i++] = 5;
        voxel.indices[i++] = 7;
        voxel.indices[i++] = 3;

        right_normal = normal;
        right_constant = -distance_from_plane( &origin, &right_normal, 0.0 )
                         - max_dist;

        left_normal = normal;
        left_constant = -distance_from_plane( &origin, &right_normal, 0.0 )
                        -min_dist;

        SCALE_VECTOR( right_normal, right_normal, -1.0 );
        right_constant = - right_constant;
    }

    for_less( i, 0, n_tag_points )
    {
        fill_Point( point, tags1[i][X], tags1[i][Y], tags1[i][Z] );

        dist = DOT_POINT_VECTOR( point, normal ) -
               DOT_POINT_VECTOR( origin, normal );

        if( dist >= min_dist && dist <= max_dist )
        {
            /*--- increase the memory allocation of the tag points */

            SET_ARRAY_SIZE( new_tags1, n_new_tags, n_new_tags+1, 10 );
            ALLOC( new_tags1[n_new_tags], 3 );

            SET_ARRAY_SIZE( new_weights, n_new_tags, n_new_tags+1, 10 );
            SET_ARRAY_SIZE( new_structure_ids, n_new_tags, n_new_tags+1, 10 );
            SET_ARRAY_SIZE( new_patient_ids, n_new_tags, n_new_tags+1, 10 );

            SET_ARRAY_SIZE( new_labels, n_new_tags, n_new_tags+1, 10 );
            ALLOC( new_labels[n_new_tags], strlen(labels[i])+1 );

            /*--- copy from the input tags to the new tags */

            new_tags1[n_new_tags][0] = tags1[i][0];
            new_tags1[n_new_tags][1] = tags1[i][1];
            new_tags1[n_new_tags][2] = tags1[i][2];

            if( n_volumes == 2 )
            {
                SET_ARRAY_SIZE( new_tags2, n_new_tags, n_new_tags+1, 10 );
                ALLOC( new_tags2[n_new_tags], 3 );

                new_tags2[n_new_tags][0] = tags2[i][0];
                new_tags2[n_new_tags][1] = tags2[i][1];
                new_tags2[n_new_tags][2] = tags2[i][2];
            }

            new_weights[n_new_tags] = weights[i];
            new_structure_ids[n_new_tags] = structure_ids[i];
            new_patient_ids[n_new_tags] = patient_ids[i];
            (void) strcpy( new_labels[n_new_tags], labels[i] );

            /*--- increment the number of new tags */

            ++n_new_tags;
        }

        if( volume_desired )
        {
            get_voxel_corners( vol, tags1[i][X], tags1[i][Y], tags1[i][Z],
                               voxel.points );
            clip_polygons_to_plane( &voxel, &left_normal, left_constant, &tmp );
            clip_polygons_to_plane( &tmp, &right_normal, right_constant,
                                    &clipped );
            delete_polygons( &tmp );
            volume += get_closed_polyhedron_volume( &clipped );
            delete_polygons( &clipped );
        }
    }

    if( volume_desired )
    {
        print( "Total volume inside planes: %g\n", volume );
    }

    /*--- output the new tags, the subset of the input tags */

    if( output_tag_file( output_tags_filename, "Removed negative X's",
                         n_volumes, n_new_tags, new_tags1, new_tags2,
                         new_weights, new_structure_ids,
                         new_patient_ids, new_labels ) != OK )
        return( 1 );

    free_tag_points( n_volumes, n_new_tags, new_tags1, new_tags2,
                     new_weights, new_structure_ids, new_patient_ids,
                     new_labels );

    if( create_plane )
    {
        n_objects = 0;

        object = create_object( POLYGONS );
        create_plane_from_points( points, min_dist, get_polygons_ptr(object) );
        add_object_to_list( &n_objects, &object_list, object );

        object = create_object( POLYGONS );
        create_plane_from_points( points, max_dist, get_polygons_ptr(object) );
        add_object_to_list( &n_objects, &object_list, object );

        (void) output_graphics_file( plane_filename, ASCII_FORMAT,
                                     n_objects, object_list );
    }

    return( 0 );
}

private  void    find_plane(
    Point      points[],
    Vector     *normal )
{
    Vector   v1, v2;

    SUB_POINTS( v1, points[1], points[0] );
    SUB_POINTS( v2, points[2], points[0] );
    CROSS_VECTORS( *normal, v1, v2 );
}

private  void  get_xy_pos(
    Vector  x_axis,
    Vector  y_axis,
    Real    x,
    Real    y,
    Point   *point )
{
    Vector   x_offset, y_offset, v;

    SCALE_VECTOR( x_offset, x_axis, x );
    SCALE_VECTOR( y_offset, y_axis, y );
    ADD_VECTORS( v, x_offset, y_offset );
    CONVERT_VECTOR_TO_POINT( *point, v );
}

private  void  create_plane_from_points(
    Point             points[],
    Real              dist,
    polygons_struct   *polygons )
{
    int      i;
    Vector   v1, v2, v3, offset;
    Real     x_min, x_max, y_min, y_max, x, y, dx, dy;
    Point    point;

    SUB_POINTS( v1, points[1], points[0] );
    SUB_POINTS( v2, points[2], points[0] );
    CROSS_VECTORS( v3, v1, v2 );
    CROSS_VECTORS( v2, v3, v1 );

    NORMALIZE_VECTOR( v1, v1 );
    NORMALIZE_VECTOR( v2, v2 );
    NORMALIZE_VECTOR( v3, v3 );

    SCALE_VECTOR( offset, v3, dist );

    x_min = 0.0;
    x_max = 0.0;
    y_min = 0.0;
    y_max = 0.0;

    for_less( i, 0, 3 )
    {
        x = DOT_VECTORS( v1, points[i] );
        y = DOT_VECTORS( v2, points[i] );

        if( i == 0 || x < x_min )
            x_min = x;
        if( i == 0 || x > x_max )
            x_max = x;
        if( i == 0 || y < y_min )
            y_min = y;
        if( i == 0 || y > y_max )
            y_max = y;
    }

    initialize_polygons( polygons, WHITE, NULL );

    polygons->n_points = 4;
    ALLOC( polygons->points, polygons->n_points );
    ALLOC( polygons->normals, polygons->n_points );

    polygons->n_items = 1;
    ALLOC( polygons->end_indices, polygons->n_items );
    polygons->end_indices[0] = 4;
    ALLOC( polygons->indices, 4 );
    polygons->indices[0] = 0;
    polygons->indices[1] = 1;
    polygons->indices[2] = 2;
    polygons->indices[3] = 3;

    dx = x_max - x_min;
    dy = y_max - y_min;

    get_xy_pos( v1, v2, x_min - X_FACTOR * dx, y_min - Y_FACTOR * dy, &point );
    polygons->points[0] = point;
    polygons->normals[0] = v3;

    get_xy_pos( v1, v2, x_max + X_FACTOR * dx, y_min - Y_FACTOR * dy, &point );
    polygons->points[1] = point;
    polygons->normals[1] = v3;

    get_xy_pos( v1, v2, x_max + X_FACTOR * dx, y_max + Y_FACTOR * dy, &point );
    polygons->points[2] = point;
    polygons->normals[2] = v3;

    get_xy_pos( v1, v2, x_min - X_FACTOR * dx, y_max + Y_FACTOR * dy, &point );
    polygons->points[3] = point;
    polygons->normals[3] = v3;

    for_less( i, 0, 4 )
    {
        ADD_POINT_VECTOR( polygons->points[i], polygons->points[i], offset );
    }
}

private  void  get_voxel_corners(
    Volume    volume,
    Real      x,
    Real      y,
    Real      z,
    Point     points[] )
{
    int    i, j, k;
    Real   v[MAX_DIMENSIONS], voxel[MAX_DIMENSIONS];
    Real   xw, yw, zw;

    convert_3D_world_to_voxel( volume, x, y, z,
                                &voxel[0], &voxel[1], &voxel[2] );

    for_less( i, 0, 2 )
    {
        v[0] = voxel[0] + (Real) i - 0.5;
        for_less( j, 0, 2 )
        {
            v[1] = voxel[1] + (Real) j - 0.5;
            for_less( k, 0, 2 )
            {
                v[2] = voxel[2] + (Real) k - 0.5;

                convert_voxel_to_world( volume, v, &xw, &yw, &zw );
                fill_Point( points[i*4+j*2+k], xw, yw, zw );
            }
        }
    }
}
