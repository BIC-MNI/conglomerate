#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  map_colours_to_sphere_topology(
    Volume            volume,
    polygons_struct   *polygons,
    Point             *origin,
    Point             *equator );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               input_filename, output_filename, volume_filename;
    Volume               volume;
    int                  n_objects, sizes[MAX_DIMENSIONS];
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;
    Real                 origin_x, origin_y, origin_z;
    Real                 equator_x, equator_y, equator_z;
    Point                origin, equator;
    STRING               dim_names[4] = { MIxspace,
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

    (void) get_real_argument( 1.0, &origin_x );
    (void) get_real_argument( 0.0, &origin_y );
    (void) get_real_argument( 0.0, &origin_z );
    (void) get_real_argument( 0.0, &equator_x );
    (void) get_real_argument( 1.0, &equator_y );
    (void) get_real_argument( 0.0, &equator_z );

    fill_Point( origin, origin_x, origin_y, origin_z );
    fill_Point( equator, equator_x, equator_y, equator_z );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
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
                      TRUE, &volume, &options ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    if( sizes[3] != 3 )
    {
        print_error( "Volume must be RGB volume.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );
    REALLOC( polygons->colours, polygons->n_points );
    polygons->colour_flag = PER_VERTEX_COLOURS;

    map_colours_to_sphere_topology( volume, polygons, &origin, &equator );

    (void) output_graphics_file( output_filename, format, n_objects,
                                 object_list );

    return( 0 );
}

private  void  convert_sphere_point_to_2d(
    Point   *point,
    Real    *u,
    Real    *v )
{
    Real   angle_up, angle_around;
    Point  up_dir, hor_dir, vert_dir;

    fill_Point( hor_dir, 1.0, 0.0, 0.0 );
    fill_Point( vert_dir, 0.0, 1.0, 0.0 );
    fill_Point( up_dir, 0.0, 0.0, 1.0 );

    angle_up = compute_clockwise_rotation( DOT_POINT_VECTOR(*point,up_dir),
                                           DOT_POINT_VECTOR(*point,vert_dir) );

    if( angle_up > PI )
        angle_up = 2 * PI - angle_up;

    *v = angle_up / PI;

    angle_around = compute_clockwise_rotation(
                         DOT_POINT_VECTOR(*point,hor_dir),
                         DOT_POINT_VECTOR(*point,vert_dir) );

    *u = angle_around / (2.0*PI);
}

private  void  convert_uv_to_volume_position(
    int      sizes[],
    Real     u,
    Real     v,
    Real     voxel[] )
{
    voxel[X] = u * (Real) (sizes[X]-1);
    voxel[Y] = v * (Real) (sizes[Y]-1);
    voxel[Z] = 0.0;
    voxel[3] = 0.0;
}

private  void  map_colours_to_sphere_topology(
    Volume            volume,
    polygons_struct   *polygons,
    Point             *origin,
    Point             *equator )
{
    int              point_index, sizes[MAX_DIMENSIONS];
    Real             u, v, voxel[MAX_DIMENSIONS], value[3];
    Real             min_value, max_value, r, g, b;
    Point            centre;
    polygons_struct  unit_sphere;
    BOOLEAN          interpolating[4] = { TRUE, TRUE, TRUE, FALSE };

    fill_Point( centre, 0.0, 0.0, 0.0 );

    get_volume_real_range( volume, &min_value, &max_value );
    get_volume_sizes( volume, sizes );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               polygons->n_items, &unit_sphere );

    for_less( point_index, 0, polygons->n_points )
    {
        convert_sphere_point_to_2d( &unit_sphere.points[point_index], &u, &v );

        convert_uv_to_volume_position( sizes, u, v, voxel );

        (void) evaluate_volume( volume, voxel, interpolating, 0, FALSE, 0.0,
                                value, NULL, NULL );

        r = (value[0] - min_value) / (max_value - min_value);
        g = (value[1] - min_value) / (max_value - min_value);
        b = (value[2] - min_value) / (max_value - min_value);

        polygons->colours[point_index] = make_Colour_0_1( r, g, b );
    }
}
