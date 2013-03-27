#include  <volume_io.h>
#include  <special_geometry.h>

#define  X_FACTOR  5.0
#define  Y_FACTOR  40.0

private  void   get_plane_normal(
    VIO_Vector    *base_normal,
    VIO_Real      angle,
    VIO_Vector    *normal );
private  void  create_plane(
    VIO_Point             *origin,
    VIO_Vector            *normal,
    VIO_Real              size,
    polygons_struct   *polygons );
private  VIO_Real  get_voxel_plane_volume(
    VIO_Volume    volume,
    VIO_Real      x,
    VIO_Real      y,
    VIO_Real      z,
    VIO_Vector    *left_normal,
    VIO_Real      left_constant,
    VIO_Vector    *right_normal,
    VIO_Real      right_constant );
private  int  get_voxel_plane_status(
    VIO_Volume    volume,
    VIO_Real      x,
    VIO_Real      y,
    VIO_Real      z,
    VIO_Vector    *left_normal,
    VIO_Real      left_constant,
    VIO_Vector    *right_normal,
    VIO_Real      right_constant );

#define  DEFAULT_ANGLE  30.0
#define  GRID           80

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_BOOL              create_planes;
    VIO_Real                 separations[MAX_DIMENSIONS], current_angle;
    VIO_Real                 angle_increment;
    char                 *three_tags_filename, *input_tags_filename;
    char                 *output_tags_filename, *plane_filename;
    char                 *volume_filename;
    VIO_Real                 min_dist, max_dist, dist, voxel_volume;
    VIO_Real                 sum_x, sum_y, sum_z, sum_yy, sum_yz, sum_zz;
    int                  i, n_objects, plane_status, p;
    VIO_Point                origin, point, centroid;
    VIO_Vector               normal, plane_normal;
    VIO_Vector               left_normal, right_normal;
    VIO_Real                 left_constant, right_constant;
    object_struct        *object, **object_list;
    int                  n_volumes, n_tag_points, *structure_ids, *patient_ids;
    VIO_Real                 **tags1, **tags2, *weights, volume, true_volume;
    char                 **labels;
    progress_struct      progress;
    polygons_struct      voxel, tmp, clipped;
    VIO_Volume               vol;
    volume_input_struct  volume_input;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &input_tags_filename ) )
    {
        print( "Usage:  %s  volume.mnc input.tag  [angle]\n" );
        return( 1 );
    }

    (void) get_real_argument( DEFAULT_ANGLE, &angle_increment;
    create_planes = get_string_argument( "plane", &plane_filename );

    if( input_tag_file( input_tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, &weights, &structure_ids,
                        &patient_ids, &labels ) != VIO_OK )
        return( 1 );

    if( start_volume_input( volume_filename, 3, XYZ_dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &vol, (minc_input_options *) NULL,
                            &volume_input ) != VIO_OK )
        return( 1 );

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

    fill_Point( centroid, sum_x / (VIO_Real) n_tag_points,
                          sum_y / (VIO_Real) n_tag_points,
                          sum_z / (VIO_Real) n_tag_points );

    if( sum_yy > 0.0 )
    {
        fill_Vector( normal, 0.0, -sum_yz / sum_yy, 1.0 );
    }
    else if( sum_zz > 0.0 )
    {
        fill_Vector( normal, 0.0, 1.0, -sum_yz / sum_zz );
    }
    else
        handle_internal_error( "Oh oh\n" );

    if( Vector_y( normal ) < 0.0 )
        SCALE_VECTOR( normal, normal, -1.0 );

    NORMALIZE_VECTOR( normal, normal );

    get_volume_separations( vol, separations );

    voxel_volume = separations[0] * separations[1] * separations[2];

    plane_constant = -distance_from_plane( &centroid, &normal, 0.0 );

    initialize_progress_report( &progress, FALSE, 360.0 / angle_increment,
                                "Clipping" );

    current_angle = 0.0;
    while( current_angle < 360.0 )
    {
        get_plane_normal( &normal, current_angle, &left_normal );
        left_constant = plane_constant;

        get_plane_normal( &normal, current_angle + angle_increment,
                          &right_normal );
        SCALE_VECTOR( right_normal, right_normal, -1.0 );
        right_constant = -plane_constant;

        volume = 0.0;

        for_less( i, 0, n_tag_points )
        {
            plane_status = get_voxel_plane_status( vol,
                                 tags1[i][X], tags1[i][Y], tags1[i][Z],
                                 &left_normal, left_constant,
                                 &right_normal, right_constant );

            if( plane_status == 1 )
            {
                volume += voxel_volume;
            }
            else if( plane_status == 0 )
            {
                volume += get_voxel_plane_volume( vol,
                                     tags1[i][X], tags1[i][Y], tags1[i][Z],
                                     &left_normal, left_constant,
                                     &right_normal, right_constant );
            }
        }

        print( "Angle: %.0g to %.0g,    VIO_Volume: %g\n",
               current_angle,
               current_angle + angle_increment, volume );

        current_angle += angle_increment;

        update_progress_report( &progress,
                                ROUND( current_angle / angle_increment ) );
    }

    terminate_progress_report( &progress );

    if( create_planes )
    {
        n_objects = 0;

        current_angle = 0.0;
        while( current_angle < 180.0 )
        {
            get_plane_normal( &normal, current_angle, &plane_normal );

            object = create_object( POLYGONS );
            create_plane( &centroid, &plane_normal, 100.0,
                          get_polygons_ptr(object) );
            add_object_to_list( &n_objects, &object_list, object );

            current_angle += angle;
        }

        (void) output_graphics_file( plane_filename, ASCII_FORMAT,
                                     n_objects, object_list );
    }

    return( 0 );
}

private  void  create_plane(
    VIO_Point             *origin,
    VIO_Vector            *normal,
    VIO_Real              size,
    polygons_struct   *polygons )
{
    int      i;
    VIO_Vector   v1, v2, offset;
    VIO_Real     x_min, x_max, y_min, y_max, x, y, dx, dy;
    VIO_Point    point;

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

private  VIO_Real  get_voxel_plane_volume(
    VIO_Volume    volume,
    VIO_Real      x,
    VIO_Real      y,
    VIO_Real      z,
    VIO_Vector    *left_normal,
    VIO_Real      left_constant,
    VIO_Vector    *right_normal,
    VIO_Real      right_constant )
{
    int    i, j, axis, a1, a2;
    VIO_Real   v[MAX_DIMENSIONS], voxel[MAX_DIMENSIONS];
    VIO_Real   separations[MAX_DIMENSIONS], max_axis_len;
    VIO_Real   l1, l2, r1, r2;
    VIO_Real   xw, yw, zw, vol, alpha_l, alpha_r, t_min, t_max;
    VIO_Point  point;

    get_volume_separations( volume, separations );

    convert_3D_world_to_voxel( volume, x, y, z,
                                &voxel[0], &voxel[1], &voxel[2] );

    voxel[0] = ROUND( voxel[0] );
    voxel[1] = ROUND( voxel[1] );
    voxel[2] = ROUND( voxel[2] );

    max_axis_len = MAX3( VIO_ABS( Vector_x(*left_normal) ),
                         VIO_ABS( Vector_y(*left_normal) ),
                         VIO_ABS( Vector_z(*left_normal) ) );
    if( VIO_ABS( Vector_x(*left_normal) ) == max_axis_len )
        axis = X;
    else if( VIO_ABS( Vector_y(*left_normal) ) == max_axis_len )
        axis = Y;
    else
        axis = Z;

    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    vol = 0.0;

    for_less( i, 0, GRID )
    {
        v[a1] = voxel[a1] + ((VIO_Real) i + 0.5) / (VIO_Real) GRID - 0.5;
        for_less( j, 0, GRID )
        {
            v[a2] = voxel[a2] + ((VIO_Real) j + 0.5) / (VIO_Real) GRID - 0.5;

            v[axis] = voxel[axis] - 0.5;
            convert_voxel_to_world( volume, v, &xw, &yw, &zw );
            fill_Point( point, xw, yw, zw );
            l1 = distance_from_plane( &point, left_normal, left_constant );
            r1 = distance_from_plane( &point, right_normal, right_constant );

            v[axis] = voxel[axis] + 0.5;
            convert_voxel_to_world( volume, v, &xw, &yw, &zw );
            fill_Point( point, xw, yw, zw );
            l2 = distance_from_plane( &point, left_normal, left_constant );
            r2 = distance_from_plane( &point, right_normal, right_constant );

            if( l1 == l2 || r1 == r2 )
            {
                if( l1 >= 0.0 && r1 >= 0.0 )
                    vol += 1.0;
            }
            else
            {
                alpha_l = l1 / (l1 - l2);
                alpha_r = r1 / (r1 - r2);

                t_min = 0.0;
                t_max = 1.0;

                if( l1 < l2 )
                    t_min = MAX( t_min, alpha_l );
                else
                    t_max = MIN( t_max, alpha_l );

                if( r1 < r2 )
                    t_min = MAX( t_min, alpha_r );
                else
                    t_max = MIN( t_max, alpha_r );

                if( t_min < t_max )
                    vol += t_max - t_min;
            }
        }
    }

    vol = (VIO_Real) vol / (VIO_Real) (GRID * GRID) *
          separations[0] * separations[1] * separations[2];

    return( vol );
}

private  int  get_voxel_plane_status(
    VIO_Volume    volume,
    VIO_Real      x,
    VIO_Real      y,
    VIO_Real      z,
    VIO_Vector    *left_normal,
    VIO_Real      left_constant,
    VIO_Vector    *right_normal,
    VIO_Real      right_constant )
{
    VIO_Real   separations[MAX_DIMENSIONS];
    VIO_Real   l, r, max_dist;
    VIO_Point  point;

    get_volume_separations( volume, separations );

    max_dist = separations[0] * separations[0] +
               separations[1] * separations[1] +
               separations[2] * separations[2];

    fill_Point( point, x, y, z );

    l = distance_from_plane( &point, left_normal, left_constant );
    r = distance_from_plane( &point, right_normal, right_constant );

    if( l < 0.0 && l * l >= max_dist )
        return( -1 );
    if( r < 0.0 && r * r >= max_dist )
        return( -1 );
    if( l > 0.0 && l * l >= max_dist &&
        r > 0.0 && r * r >= max_dist )
    {
        return( 1 );
    }
    else
        return( 0 );
}

private  void   get_plane_normal(
    VIO_Vector    *base_normal,
    VIO_Real      angle,
    VIO_Vector    *normal )
{
    VIO_Real       x, y, z;
    Transform  transform;

    make_rotation_transform( angle * DEG_TO_RAD, X, &transform );
    transform_vector( &transform,
                      Vector_x(*base_normal),
                      Vector_y(*base_normal),
                      Vector_z(*base_normal),
                      &x, &y, &z );
    fill_Vector( *normal, x, y, z );
}
