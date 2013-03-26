#include  <volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.4

static  int   make_grid_lines(
    polygons_struct   *polygons,
    BOOLEAN           origin_specified,
    VIO_Point             *origin,
    VIO_Real              origin_u,
    VIO_Real              origin_v,
    VIO_Real              rot_angle,
    int               n_long,
    int               n_lat,
    object_struct     ***objects );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR               input_filename, output_filename;
    int                  n_objects;
    int                  n_long, n_lat, n_created;
    VIO_File_formats         format;
    object_struct        **object_list, **objects;
    polygons_struct      *polygons;
    VIO_Real                 origin_x, origin_y, origin_z;
    VIO_Real                 origin_u, origin_v, rot_angle;
    BOOLEAN              origin_specified;
    VIO_Point                origin;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &n_long ) ||
        !get_int_argument( 0, &n_lat ) )
    {
        print_error( "Usage: %s  src.obj dest.obj nlong nlat\n", argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 1.0, &origin_x );
    (void) get_real_argument( 0.0, &origin_y );
    (void) get_real_argument( 0.0, &origin_z );
    (void) get_real_argument( 0.0, &origin_u );
    (void) get_real_argument( 1.0, &origin_v );
    origin_specified = get_real_argument( 0.0, &rot_angle );

    fill_Point( origin, origin_x, origin_y, origin_z );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS ||
        !is_this_tetrahedral_topology(get_polygons_ptr(object_list[0])) )
    {
        print_error( "Must contain exactly one sphere topology object.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    n_created = make_grid_lines( polygons, origin_specified, &origin,
                                 origin_u, origin_v, rot_angle,
                                 n_long, n_lat, &objects );

    (void) output_graphics_file( output_filename, format, n_created, objects );

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

    make_rotation_about_axis( &axis, -angle * DEG_TO_RAD, &about_point );

    create_two_orthogonal_vectors( &axis, &vert, &up );
    NORMALIZE_VECTOR( vert, vert );
    NORMALIZE_VECTOR( up, up );

    vert_angle = INTERPOLATE( v, -PI/2.0, PI/2.0 );

    make_rotation_about_axis( &vert, vert_angle, &vert_rot );

    transform_point( &vert_rot,
                     (VIO_Real) Vector_x(up),
                     (VIO_Real) Vector_y(up),
                     (VIO_Real) Vector_z(up), &x, &y, &z );

    fill_Vector( up_transformed, x, y, z );
    NORMALIZE_VECTOR( up_transformed, up_transformed );

    z_angle = -2.0 * PI * u;

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

static  int   make_grid_lines(
    polygons_struct   *polygons,
    BOOLEAN           origin_specified,
    VIO_Point             *origin,
    VIO_Real              origin_u,
    VIO_Real              origin_v,
    VIO_Real              rot_angle,
    int               n_long,
    int               n_lat,
    object_struct     ***objects )
{
    VIO_Real             angle, c, s, x, y, z;
    VIO_Point            centre, unit_origin;
    polygons_struct  unit_sphere;
    lines_struct     *lines;
    VIO_Vector           normal;
    int              n_objects, l;
    VIO_Transform        transform, inverse;
    VIO_progress_struct  progress;

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               polygons->n_items, &unit_sphere );

    create_polygons_bintree( &unit_sphere,
                      VIO_ROUND( (VIO_Real)unit_sphere.n_items * BINTREE_FACTOR ) );

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

    compute_transform_inverse( &transform, &inverse );

    n_objects = 0;

    initialize_progress_report( &progress, FALSE, n_long, "Longitudes" );

    for_less( l, 0, n_long )
    {
        angle = 2.0 * PI * (VIO_Real) l / (VIO_Real) n_long;
        c = cos( angle );
        s = sin( angle );

        fill_Vector( normal, c, -s, 0.0 );

        transform_point( &inverse,
                         (VIO_Real) Point_x(normal),
                         (VIO_Real) Point_y(normal),
                         (VIO_Real) Point_z(normal), &x, &y, &z );

        fill_Vector( normal, x, y, z );
        NORMALIZE_VECTOR( normal, normal );

        SET_ARRAY_SIZE( *objects, n_objects, n_objects+1, DEFAULT_CHUNK_SIZE );
        (*objects)[n_objects] = create_object( LINES );
        lines = get_lines_ptr( (*objects)[n_objects] );

        intersect_planes_with_polygons( &unit_sphere, &centre, &normal, lines );

        polygon_transform_points( &unit_sphere, polygons,
                                  lines->n_points,
                                  lines->points, lines->points );

        ++n_objects;

        update_progress_report( &progress, l + 1 );
    }

    terminate_progress_report( &progress );

    initialize_progress_report( &progress, FALSE, n_lat-1, "Latitudes" );

    for_less( l, 0, n_lat-1 )
    {
        angle = INTERPOLATE( (VIO_Real) (l+1) / (VIO_Real) n_lat,
                             -PI/ 2.0, PI/ 2.0 );
        z = sin( angle );

        fill_Point( centre, 0.0, 0.0, z );
        fill_Vector( normal, 0.0, 0.0, 1.0 );

        transform_point( &inverse,
                         (VIO_Real) Point_x(centre),
                         (VIO_Real) Point_y(centre),
                         (VIO_Real) Point_z(centre), &x, &y, &z );
        fill_Point( centre, x, y, z );

        transform_point( &inverse,
                         (VIO_Real) Point_x(normal),
                         (VIO_Real) Point_y(normal),
                         (VIO_Real) Point_z(normal), &x, &y, &z );
        fill_Vector( normal, x, y, z );
        NORMALIZE_VECTOR( normal, normal );

        SET_ARRAY_SIZE( *objects, n_objects, n_objects+1, DEFAULT_CHUNK_SIZE );
        (*objects)[n_objects] = create_object( LINES );
        lines = get_lines_ptr( (*objects)[n_objects] );

        intersect_planes_with_polygons( &unit_sphere, &centre, &normal, lines );

        polygon_transform_points( &unit_sphere, polygons,
                                  lines->n_points,
                                  lines->points, lines->points );

        ++n_objects;

        update_progress_report( &progress, l + 1 );
    }

    terminate_progress_report( &progress );

    return( n_objects );
}
