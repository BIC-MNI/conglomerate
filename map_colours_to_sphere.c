#include  <volume_io.h>
#include  <bicpl.h>

static  void  map_colours_to_sphere_topology(
    VIO_Volume            volume,
    int               continuity,
    polygons_struct   *polygons,
    VIO_BOOL           origin_specified,
    VIO_Point             *origin,
    VIO_Real              origin_u,
    VIO_Real              origin_v,
    VIO_Real              rot_angle );
static  void  evaluate_volume_by_voting(
    VIO_Volume    volume,
    VIO_Real      x,
    VIO_Real      y,
    VIO_Real      z,
    VIO_Real      fill_value,
    VIO_Real      values[] );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR               input_filename, output_filename, volume_filename;
    VIO_Volume               volume;
    int                  n_objects, sizes[VIO_MAX_DIMENSIONS], continuity;
    VIO_File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;
    VIO_Real                 origin_x, origin_y, origin_z;
    VIO_Real                 origin_u, origin_v, rot_angle;
    VIO_BOOL              origin_specified;
    VIO_Point                origin;
    VIO_STR               dim_names[4] = { MIxspace,
                                          MIyspace,
                                          MIzspace,
                                          MIvector_dimension };
    minc_input_options   options;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  src.obj src.mnc dest.obj\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -2, &continuity );

    (void) get_real_argument( 1.0, &origin_x );
    (void) get_real_argument( 0.0, &origin_y );
    (void) get_real_argument( 0.0, &origin_z );
    (void) get_real_argument( 0.0, &origin_u );
    (void) get_real_argument( 1.0, &origin_v );
    origin_specified = get_real_argument( 0.0, &rot_angle );

    fill_Point( origin, origin_x, origin_y, origin_z );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK )
        return( 1 );

    if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS ||
        !is_this_tetrahedral_topology(get_polygons_ptr(object_list[0])) )
    {
        print_error( "Must contain exactly one sphere topology object.\n" );
        return( 1 );
    }

    set_default_minc_input_options( &options );
    set_minc_input_vector_to_scalar_flag( &options, FALSE );

    if( input_volume( volume_filename, 4, dim_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, &options ) != VIO_OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    if( sizes[3] != 3 && sizes[3] != 4 )
    {
        print_error( "VIO_Volume must be RGB volume.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );
    REALLOC( polygons->colours, polygons->n_points );
    polygons->colour_flag = PER_VERTEX_COLOURS;

    map_colours_to_sphere_topology( volume, continuity,
                                    polygons, origin_specified,
                                    &origin, origin_u, origin_v, rot_angle );

    (void) output_graphics_file( output_filename, format, n_objects,
                                 object_list );

    return( 0 );
}

static  void  get_sphere_transform(
    VIO_Point      *pos,
    VIO_Real       u,
    VIO_Real       v,
    VIO_Real       angle,
    VIO_Transform  *transform )
{
    VIO_Real       z_angle, vert_angle, x, y, z;
    VIO_Vector     axis, vert, up, vert_transformed, up_transformed;
    VIO_Vector     axis_transformed;
    VIO_Point      origin;
    VIO_Transform  about_point, vert_rot, z_rot, change;

    CONVERT_POINT_TO_VECTOR( axis, *pos );

    make_rotation_about_axis( &axis, -angle * VIO_DEG_TO_RAD, &about_point );

    create_two_orthogonal_vectors( &axis, &vert, &up );
    NORMALIZE_VECTOR( vert, vert );
    NORMALIZE_VECTOR( up, up );

    vert_angle = VIO_INTERPOLATE( v, -M_PI/2.0, M_PI/2.0 );

    make_rotation_about_axis( &vert, vert_angle, &vert_rot );

    transform_point( &vert_rot,
                     (VIO_Real) Vector_x(up),
                     (VIO_Real) Vector_y(up),
                     (VIO_Real) Vector_z(up), &x, &y, &z );

    fill_Vector( up_transformed, x, y, z );
    NORMALIZE_VECTOR( up_transformed, up_transformed );

    z_angle = -2.0 * M_PI * u;

    make_rotation_about_axis( &up_transformed, z_angle, &z_rot );

    transform_point( &z_rot,
                     (VIO_Real) Vector_x(vert),
                     (VIO_Real) Vector_y(vert),
                     (VIO_Real) Vector_z(vert), &x, &y, &z );
    fill_Vector( vert_transformed, x, y, z );
    NORMALIZE_VECTOR( vert_transformed, vert_transformed );
    CROSS_VECTORS( axis_transformed, vert_transformed, up_transformed );
    NORMALIZE_VECTOR( axis_transformed, axis_transformed );

    fill_Point( origin, 0.0, 0.0, 0.0 );
    make_change_from_bases_transform( &origin, &axis_transformed,
                                      &vert_transformed, &up_transformed,
                                      &change );

    concat_transforms( transform, &about_point, &change );
}

static  void  convert_sphere_point_to_2d(
    VIO_Transform  *transform,
    VIO_Point      *point,
    VIO_Real       *u,
    VIO_Real       *v )
{
    VIO_Real    x, y, z;
    VIO_Real    angle_up, angle_around;

    transform_point( transform,
                     (VIO_Real) Point_x(*point),
                     (VIO_Real) Point_y(*point),
                     (VIO_Real) Point_z(*point), &x, &y, &z );

    angle_up = acos( z );

    *v = 1.0 - angle_up / M_PI;

    angle_around = compute_clockwise_rotation( x, -y );

    *u = angle_around / (2.0*M_PI);
}

static  void  convert_uv_to_volume_position(
    int      sizes[],
    VIO_Real     u,
    VIO_Real     v,
    VIO_Real     voxel[] )
{
    voxel[VIO_X] = u * (VIO_Real) (sizes[VIO_X]-1);
    voxel[VIO_Y] = v * (VIO_Real) (sizes[VIO_Y]-1);
    voxel[VIO_Z] = 0.0;
    voxel[3] = 0.0;
}

static  void  map_colours_to_sphere_topology(
    VIO_Volume            volume,
    int               continuity,
    polygons_struct   *polygons,
    VIO_BOOL           origin_specified,
    VIO_Point             *origin,
    VIO_Real              origin_u,
    VIO_Real              origin_v,
    VIO_Real              rot_angle )
{
    int              point_index, sizes[VIO_MAX_DIMENSIONS];
    VIO_Real             u, v, voxel[VIO_MAX_DIMENSIONS], value[4];
    VIO_Real             min_value, max_value, r, g, b;
    VIO_Point            centre, unit_origin;
    polygons_struct  unit_sphere;
    VIO_Transform        transform;
    VIO_BOOL          interpolating[4] = { TRUE, TRUE, TRUE, FALSE };

    fill_Point( centre, 0.0, 0.0, 0.0 );

    get_volume_real_range( volume, &min_value, &max_value );
    get_volume_sizes( volume, sizes );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               polygons->n_items, &unit_sphere );

    if( origin_specified )
    {
        map_point_to_unit_sphere( polygons, origin, &unit_sphere,
                                  &unit_origin );
        print( "Used unit origin: %g %g %g\n",
               Point_x(unit_origin),
               Point_y(unit_origin),
               Point_z(unit_origin) );
    }
    else
    {
        fill_Point( unit_origin, 1.0, 0.0, 0.0 );
        origin_u = 0.0;
        origin_v = 0.5;
        rot_angle = 135.0;
    }

    get_sphere_transform( &unit_origin, origin_u, origin_v, rot_angle,
                          &transform );

    convert_sphere_point_to_2d( &transform,
                                &unit_origin, &u, &v );

    print( "%g %g %g %g\n", origin_u, origin_v, u, v );

    for_less( point_index, 0, polygons->n_points )
    {
        convert_sphere_point_to_2d( &transform,
                                    &unit_sphere.points[point_index], &u, &v );

        convert_uv_to_volume_position( sizes, u, v, voxel );

        if( continuity == -2 )
        {
            evaluate_volume_by_voting( volume,
                                       voxel[VIO_X], voxel[VIO_Y], voxel[VIO_Z],
                                       0.0, value );
        }
        else
        {
            (void) evaluate_volume( volume, voxel, interpolating, continuity,
                                    FALSE, 0.0, value, NULL, NULL );
        }

        r = (value[0] - min_value) / (max_value - min_value);
        g = (value[1] - min_value) / (max_value - min_value);
        b = (value[2] - min_value) / (max_value - min_value);

        polygons->colours[point_index] = make_Colour_0_1( r, g, b );
    }
}

static  void  evaluate_volume_by_voting(
    VIO_Volume    volume,
    VIO_Real      x,
    VIO_Real      y,
    VIO_Real      z,
    VIO_Real      fill_value,
    VIO_Real      values[] )
{
    int      id[2][2][2], sizes[VIO_N_DIMENSIONS];
    VIO_Real     corners[2][2][2][4], x_frac, y_frac, z_frac;
    int      i, j, k, tx, ty, tz, dx, dy, dz;
    int      clas, n_classes, coef_ind;
    VIO_Real     coefs[8], best_value, interp[4], value;

    get_volume_sizes( volume, sizes );

    i = VIO_FLOOR( x );
    j = VIO_FLOOR( y );
    k = VIO_FLOOR( z );

    for_less( dx, 0, 2 )
    {
        tx = i + dx;
        for_less( dy, 0, 2 )
        {
            ty = j + dy;
            for_less( dz, 0, 2 )
            {
                tz = k + dz;

                id[dx][dy][dz] = -1;
                if( tx < 0 || tx >= sizes[VIO_X] ||
                    ty < 0 || ty >= sizes[VIO_Y] ||
                    tz < 0 || tz >= sizes[VIO_Z] )
                {
                    corners[dx][dy][dz][0] = fill_value;
                    corners[dx][dy][dz][1] = fill_value;
                    corners[dx][dy][dz][2] = fill_value;
                }
                else
                {
                    corners[dx][dy][dz][0] = get_volume_real_value( volume,
                                                 tx, ty, tz, 0, 0 );
                    corners[dx][dy][dz][1] = get_volume_real_value( volume,
                                                 tx, ty, tz, 1, 0 );
                    corners[dx][dy][dz][2] = get_volume_real_value( volume,
                                                 tx, ty, tz, 2, 0 );
                }
            }
        }
    }

    n_classes = 0;
    for_less( dx, 0, 2 )
    for_less( dy, 0, 2 )
    for_less( dz, 0, 2 )
    {
        if( id[dx][dy][dz] >= 0 )
            continue;

        for_less( tx, 0, 2 )
        for_less( ty, 0, 2 )
        for_less( tz, 0, 2 )
        {
            if( corners[dx][dy][dz][0] == corners[tx][ty][tz][0] &&
                corners[dx][dy][dz][1] == corners[tx][ty][tz][1] &&
                corners[dx][dy][dz][2] == corners[tx][ty][tz][2] )
                id[tx][ty][tz] = n_classes;
        }
        ++n_classes;
    }

    x_frac = x - (VIO_Real) i;
    y_frac = y - (VIO_Real) j;
    z_frac = z - (VIO_Real) k;

    best_value = 0.0;

    for_less( clas, 0, n_classes )
    {
        coef_ind = 0;
        for_less( dx, 0, 2 )
        for_less( dy, 0, 2 )
        for_less( dz, 0, 2 )
        {
            if( id[dx][dy][dz] == clas )
            {
                coefs[coef_ind] = 1.0;
                interp[0] = corners[dx][dy][dz][0];
                interp[1] = corners[dx][dy][dz][1];
                interp[2] = corners[dx][dy][dz][2];
            }
            else
                coefs[coef_ind] = 0.0;
            ++coef_ind;
        }

        evaluate_trivariate_interpolating_spline( x_frac, y_frac, z_frac,
                                                  2, coefs, 0, &value );

        if( clas == 0 || value > best_value )
        {
            best_value = value;
            values[0] = interp[0];
            values[1] = interp[1];
            values[2] = interp[2];
        }
    }
}
