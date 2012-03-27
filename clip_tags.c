#include  <volume_io.h>
#include  <special_geometry.h>

#define  X_FACTOR  5.0
#define  Y_FACTOR  5.0

#define  GRID  40

private  void    find_plane(
    Point      points[],
    Vector     *normal );
private  void  create_plane_from_points(
    Point             points[],
    Real              dist,
    polygons_struct   *polygons );
private  void   get_plane_normal(
    Vector    *base_normal,
    Real      angle,
    Vector    *normal );
private  Real  get_voxel_plane_volume(
    Volume    volume,
    Real      x,
    Real      y,
    Real      z,
    int       n_planes,
    Vector    normals[],
    Real      constants[] );
private  int  get_voxel_plane_status(
    Volume    volume,
    Real      x,
    Real      y,
    Real      z,
    int       n_planes,
    Vector    normals[],
    Real      constants[] );
private  void  create_segment_plane(
    Point             *origin,
    Vector            *normal,
    Real              size,
    polygons_struct   *polygons );

int  main(
    int   argc,
    char  *argv[] )
{
    BOOLEAN              create_plane, volume_desired, segment;
    Real                 sum_x, sum_y, sum_z, sum_yy, sum_yz, sum_zz;
    Real                 separations[MAX_DIMENSIONS], angle;
    STRING               three_tags_filename, input_tags_filename;
    STRING               output_tags_filename, plane_filename;
    STRING               volume_filename;
    Real                 min_dist, max_dist, dist, voxel_volume;
    Real                 angle_increment, angle_offset;
    int                  i, n_objects, plane_status;
    Point                points[3], origin, point, centroid;
    Vector               normal, plane_normal;
    Vector               left_normal, right_normal;
    Real                 left_constant, right_constant;
    object_struct        *object, **object_list;
    Vector               princ_normal, normals[4];
    Real                 plane_constants[4];
    int                  n_volumes, n_tag_points, *structure_ids, *patient_ids;
    Real                 **tags1, **tags2, *weights, volume;
    STRING               *labels;
    int                  n_new_tags, *new_structure_ids, *new_patient_ids;
    Real                 **new_tags1, **new_tags2, *new_weights;
    STRING               *new_labels;
#ifdef PROGRESS
    progress_struct      progress;
#endif
    Volume               vol;
    volume_input_struct  volume_input;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &three_tags_filename ) ||
        !get_string_argument( "", &input_tags_filename ) ||
        !get_string_argument( "", &output_tags_filename ) ||
        !get_real_argument( 0.0, &min_dist ) ||
        !get_real_argument( 0.0, &max_dist ) )
    {
        print( "Usage:  %s  3_tags_file  input.tag  output.tag  -distance +distance [mnc_file_if_you_want_volume]   [angle_increment]  [angle_offset] [plane_filename]\n", argv[0] );
        return( 1 );
    }

    volume_desired = get_string_argument( "", &volume_filename );
    segment = get_real_argument( 0.0, &angle_increment );
    (void) get_real_argument( 0.0, &angle_offset );
    create_plane = get_string_argument( "", &plane_filename );

    if( min_dist > max_dist )
    {
        Real  swap;

        swap = min_dist;
        min_dist = max_dist;
        max_dist = swap;
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

        get_volume_separations( vol, separations );
        voxel_volume = separations[0] * separations[1] * separations[2];
        volume = 0.0;

        right_normal = normal;
        right_constant = -distance_from_plane( &origin, &right_normal, 0.0 )
                         - max_dist;

        left_normal = normal;
        left_constant = -distance_from_plane( &origin, &right_normal, 0.0 )
                        -min_dist;

        SCALE_VECTOR( right_normal, right_normal, -1.0 );
        right_constant = - right_constant;
    }

    normals[0] = left_normal;
    plane_constants[0] = left_constant;
    normals[1] = right_normal;
    plane_constants[1] = right_constant;

#ifdef  PROGRESS
    initialize_progress_report( &progress, FALSE, n_tag_points,
                            "Clipping" );
#endif

    new_tags1 = NULL;
    new_tags2 = NULL;
    new_weights = NULL;
    new_structure_ids = NULL;
    new_patient_ids = NULL;
    new_labels = NULL;

    for_less( i, 0, n_tag_points )
    {
        fill_Point( point, tags1[i][X], tags1[i][Y], tags1[i][Z] );

        dist = distance_from_plane( &point, &normal,
                                    -DOT_POINT_VECTOR(origin,normal) );

        if( dist >= min_dist && dist <= max_dist )
        {
            /*--- increase the memory allocation of the tag points */

            SET_ARRAY_SIZE( new_tags1, n_new_tags, n_new_tags+1, 10 );
            ALLOC( new_tags1[n_new_tags], 3 );

            SET_ARRAY_SIZE( new_weights, n_new_tags, n_new_tags+1, 10 );
            SET_ARRAY_SIZE( new_structure_ids, n_new_tags, n_new_tags+1, 10 );
            SET_ARRAY_SIZE( new_patient_ids, n_new_tags, n_new_tags+1, 10 );

            SET_ARRAY_SIZE( new_labels, n_new_tags, n_new_tags+1, 10 );

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
            new_labels[n_new_tags] = create_string( labels[i] );

            /*--- increment the number of new tags */

            ++n_new_tags;
        }

        if( volume_desired )
        {
            plane_status = get_voxel_plane_status( vol,
                                 tags1[i][X], tags1[i][Y], tags1[i][Z],
                                 2, normals, plane_constants );

            if( plane_status == 1 )
            {
                volume += voxel_volume;
            }
            else if( plane_status == 0 )
            {
                volume += get_voxel_plane_volume( vol,
                                     tags1[i][X], tags1[i][Y], tags1[i][Z],
                                     2, normals, plane_constants );
            }
        }

#ifdef  PROGRESS
        update_progress_report( &progress, i+1 );
#endif
    }

#ifdef  PROGRESS
    terminate_progress_report( &progress );
#endif

    if( volume_desired )
        print( "Total volume inside planes: %g\n", volume );

    /*--- output the new tags, the subset of the input tags */

    if( output_tag_file( output_tags_filename, "Removed negative X's",
                         n_volumes, n_new_tags, new_tags1, new_tags2,
                         new_weights, new_structure_ids,
                         new_patient_ids, new_labels ) != OK )
        return( 1 );

    free_tag_points( n_volumes, n_new_tags, new_tags1, new_tags2,
                     new_weights, new_structure_ids, new_patient_ids,
                     new_labels );

    if( segment )
    {
        sum_x = 0.0;
        sum_y = 0.0;
        sum_z = 0.0;
        sum_yy = 0.0;
        sum_yz = 0.0;
        sum_zz = 0.0;

        for_less( i, 0, n_tag_points )
        {
            sum_x += tags1[i][X];
            sum_y += tags1[i][Y];
            sum_z += tags1[i][Z];

            sum_yy += tags1[i][Y] * tags1[i][Y];
            sum_yz += tags1[i][Y] * tags1[i][Z];
            sum_zz += tags1[i][Z] * tags1[i][Z];
        }

        fill_Point( centroid, sum_x / (Real) n_tag_points,
                              sum_y / (Real) n_tag_points,
                              sum_z / (Real) n_tag_points );

        if( sum_yy > 0.0 )
        {
            fill_Vector( princ_normal, 0.0, -sum_yz / sum_yy, 1.0 );
        }
        else if( sum_zz > 0.0 )
        {
            fill_Vector( princ_normal, 0.0, 1.0, -sum_yz / sum_zz );
        }
        else
            handle_internal_error( "Oh oh\n" );

        if( Vector_y( princ_normal ) < 0.0f )
            SCALE_VECTOR( princ_normal, princ_normal, -1.0 );

        NORMALIZE_VECTOR( princ_normal, princ_normal );

        angle = 0.0;

        while( angle < 360.0 )
        {
            get_plane_normal( &princ_normal, angle + angle_offset,
                              &normals[2] );

            get_plane_normal( &princ_normal,
                              angle + angle_offset + angle_increment,
                              &normals[3] );
            SCALE_VECTOR( normals[3], normals[3], -1.0 );

            plane_constants[2] = -distance_from_plane( &centroid, &normals[2],
                                                       0.0 );
            plane_constants[3] = -distance_from_plane( &centroid, &normals[3],
                                                       0.0 );
            volume = 0.0;

#ifdef  PROGRESS
            initialize_progress_report( &progress, FALSE,
                                        angle / angle_increment, "Segmenting" );
#endif

            for_less( i, 0, n_tag_points )
            {
                fill_Point( point, tags1[i][X], tags1[i][Y], tags1[i][Z] );

                plane_status = get_voxel_plane_status( vol,
                                 tags1[i][X], tags1[i][Y], tags1[i][Z],
                                 4, normals, plane_constants );

                if( plane_status == 1 )
                {
                    volume += voxel_volume;
                }
                else if( plane_status == 0 )
                {
                    volume += get_voxel_plane_volume( vol,
                                     tags1[i][X], tags1[i][Y], tags1[i][Z],
                                     4, normals, plane_constants );
                }
            }

            print( "Angle: %4.0f to %4.0f,    Volume: %g\n",
                   angle, angle + angle_increment, volume );

            angle += angle_increment;
        }

#ifdef  PROGRESS
        update_progress_report( &progress, i+1 );
#endif
    }

#ifdef  PROGRESS
    terminate_progress_report( &progress );
#endif

    if( create_plane )
    {
        n_objects = 0;

        object = create_object( POLYGONS );
        create_plane_from_points( points, min_dist, get_polygons_ptr(object) );
        add_object_to_list( &n_objects, &object_list, object );

        object = create_object( POLYGONS );
        create_plane_from_points( points, max_dist, get_polygons_ptr(object) );
        add_object_to_list( &n_objects, &object_list, object );

        if( segment )
        {
            angle = 0.0;
            while( angle < 180.0 )
            {
                get_plane_normal( &princ_normal, angle+angle_offset,
                                  &plane_normal );

                object = create_object( POLYGONS );
                create_segment_plane( &centroid, &plane_normal, 100.0,
                                      get_polygons_ptr(object) );
                if( angle == 0.0 )
                    get_polygons_ptr(object)->colours[0] = GREEN;
                else if( angle == angle_increment )
                    get_polygons_ptr(object)->colours[0] = RED;

                add_object_to_list( &n_objects, &object_list, object );

                angle += angle_increment;
            }
        }

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
    Vector   v1, v2, v3, offset, v;
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

    CONVERT_POINT_TO_VECTOR( v, points[0] );

    ADD_VECTORS( offset, offset, v );

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

private  Real  get_voxel_plane_volume(
    Volume    volume,
    Real      x,
    Real      y,
    Real      z,
    int       n_planes,
    Vector    normals[],
    Real      constants[] )
{
    int    i, j, axis, a1, a2, p;
    Real   v[MAX_DIMENSIONS], voxel[MAX_DIMENSIONS];
    Real   separations[MAX_DIMENSIONS];
    Real   d1, d2;
    Real   xw, yw, zw, vol, alpha, t_min, t_max;
    Point  point;

    get_volume_separations( volume, separations );

    convert_3D_world_to_voxel( volume, x, y, z,
                                &voxel[0], &voxel[1], &voxel[2] );

    voxel[0] = (Real) ROUND( voxel[0] );
    voxel[1] = (Real) ROUND( voxel[1] );
    voxel[2] = (Real) ROUND( voxel[2] );

    axis = X;
    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    vol = 0.0;

    for_less( i, 0, GRID )
    {
        v[a1] = voxel[a1] + ((Real) i + 0.5) / (Real) GRID - 0.5;
        for_less( j, 0, GRID )
        {
            v[a2] = voxel[a2] + ((Real) j + 0.5) / (Real) GRID - 0.5;

            t_min = 0.0;
            t_max = 1.0;

            for_less( p, 0, n_planes )
            {
                v[axis] = voxel[axis] - 0.5;
                convert_voxel_to_world( volume, v, &xw, &yw, &zw );
                fill_Point( point, xw, yw, zw );
                d1 = distance_from_plane( &point, &normals[p], constants[p] );

                v[axis] = voxel[axis] + 0.5;
                convert_voxel_to_world( volume, v, &xw, &yw, &zw );
                fill_Point( point, xw, yw, zw );
                d2 = distance_from_plane( &point, &normals[p], constants[p] );

                if( d1 == d2 )
                {
                    if( d1 < 0.0 )
                    {
                        t_min = 0.0;
                        t_max = 0.0;
                        break;
                    }
                }
                else
                {
                    alpha = d1 / (d1 - d2);

                    if( d1 < d2 )
                        t_min = MAX( t_min, alpha );
                    else
                        t_max = MIN( t_max, alpha );
                }
            }

            if( t_min < t_max )
                vol += t_max - t_min;
        }
    }

    vol = (Real) vol / (Real) (GRID * GRID) *
          separations[0] * separations[1] * separations[2];

    return( vol );
}

private  int  get_voxel_plane_status(
    Volume    volume,
    Real      x,
    Real      y,
    Real      z,
    int       n_planes,
    Vector    normals[],
    Real      constants[] )
{
    int    i;
    Real   separations[MAX_DIMENSIONS];
    Real   d, max_dist;
    Point  point;

    get_volume_separations( volume, separations );

    max_dist = separations[0] * separations[0] +
               separations[1] * separations[1] +
               separations[2] * separations[2];

    fill_Point( point, x, y, z );

    for_less( i, 0, n_planes )
    {
        d = distance_from_plane( &point, &normals[i], constants[i] );

        if( d < 0.0 && d * d >= max_dist )
            return( -1 );
        else if( d * d < max_dist )
            return( 0 );
    }

    return( 1 );
}

private  void  create_segment_plane(
    Point             *origin,
    Vector            *normal,
    Real              size,
    polygons_struct   *polygons )
{
    Vector   v1, v2;

    create_two_orthogonal_vectors( normal, &v1, &v2 );

    NORMALIZE_VECTOR( v1, v1 );
    NORMALIZE_VECTOR( v2, v2 );

    SCALE_VECTOR( v1, v1, size );
    SCALE_VECTOR( v2, v2, size );

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

    polygons->normals[0] = *normal;
    polygons->normals[1] = *normal;
    polygons->normals[2] = *normal;
    polygons->normals[3] = *normal;

    ADD_POINT_VECTOR( polygons->points[0], *origin, v1 );
    SUB_POINT_VECTOR( polygons->points[0], polygons->points[0], v2 );

    ADD_POINT_VECTOR( polygons->points[1], *origin, v1 );
    ADD_POINT_VECTOR( polygons->points[1], polygons->points[1], v2 );

    SUB_POINT_VECTOR( polygons->points[2], *origin, v1 );
    ADD_POINT_VECTOR( polygons->points[2], polygons->points[2], v2 );

    SUB_POINT_VECTOR( polygons->points[3], *origin, v1 );
    SUB_POINT_VECTOR( polygons->points[3], polygons->points[3], v2 );
}


private  void   get_plane_normal(
    Vector    *base_normal,
    Real      angle,
    Vector    *normal )
{
    Real       x, y, z;
    Transform  transform;

    make_rotation_transform( angle * DEG_TO_RAD, X, &transform );
    transform_vector( &transform,
                      (Real) Vector_x(*base_normal),
                      (Real) Vector_y(*base_normal),
                      (Real) Vector_z(*base_normal),
                      &x, &y, &z );
    fill_Vector( *normal, x, y, z );

    NORMALIZE_VECTOR( *normal, *normal );
}
