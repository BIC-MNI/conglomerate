#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s surface.obj  input.obj|input.txt [invert] output\n\
\n\
                       x1 y1 z1 u1 v1 \n\
                       x2 y2 z2 u2 v2 \n\
                       [u_min u_max v_min v_max] | \n\
                       [x3 y3 z3 x4 y4 z4 ...] \n\
     Maps an object on a surface to a flat sheet.\n\n";

    print_error( usage_str, executable );
}

private  BOOLEAN    get_sphere_transform(
    Real        x1,
    Real        y1,
    Real        z1,
    Real        u1,
    Real        v1,
    Real        x2,
    Real        y2,
    Real        z2,
    Real        u2,
    Real        v2,
    Transform   *transform );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               surface_filename, input_filename, output_filename;
    File_formats         format;
    polygons_struct      *surface, unit_sphere;
#ifdef DEBUG
    int                  i, j;
#endif
    int                  n_objects, obj, n_points;
    int                  n_surf_objects, n_args;
    int                  n_bounding_points, point;
    FILE                 *file;
    Transform            transform, inverse;
    Point                *bounding_points, *points;
    Real                 x, y, z;
    Real                 u, v, u1, v1, u2, v2, u_min, u_max, v_min, v_max;
    Real                 x1, y1, z1, x2, y2, z2, *values;
    object_struct        **objects, **surf_objects;
    Point                unit_point, centre, unit_point1, unit_point2, pt;
    progress_struct      progress;
    BOOLEAN              invert_flag, transforming_values;

    /*--- get the arguments from the command line */

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( equal_strings( output_filename, "invert" ) )
    {
        invert_flag = TRUE;
        if( !get_string_argument( NULL, &output_filename ) )
        {
            usage( argv[0] );
            return( 1 );
        }
    }
    else
        invert_flag = FALSE;

    if( !get_real_argument( 0.0, &x1 ) ||
        !get_real_argument( 0.0, &y1 ) ||
        !get_real_argument( 0.0, &z1 ) ||
        !get_real_argument( 0.0, &u1 ) ||
        !get_real_argument( 0.0, &v1 ) ||
        !get_real_argument( 0.0, &x2 ) ||
        !get_real_argument( 0.0, &y2 ) ||
        !get_real_argument( 0.0, &z2 ) ||
        !get_real_argument( 0.0, &u2 ) ||
        !get_real_argument( 0.0, &v2 ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    n_args = get_n_arguments_remaining();

    if( n_args == 4 )
    {
        if( !get_real_argument( 0.0, &u_min ) ||
            !get_real_argument( 0.0, &u_max ) ||
            !get_real_argument( 0.0, &v_min ) ||
            !get_real_argument( 0.0, &v_max ) )
        {
            usage( argv[0] );
            return( 1 );
        }
        n_bounding_points = 0;
    }
    else
    {
        if( n_args < 3 || n_args % 3 != 0 )
        {
            usage( argv[0] );
            return( 1 );
        }

        n_bounding_points = n_args / 3;
        ALLOC( bounding_points, n_bounding_points );

        for_less( point, 0, n_bounding_points )
        {
            if( !get_real_argument( 0.0, &x ) ||
                !get_real_argument( 0.0, &y ) ||
                !get_real_argument( 0.0, &z ) )
            {
                usage( argv[0] );
                return( 1 );
            }
            fill_Point( bounding_points[point], x, y, z );
        }
    }

    /*--- input the surface */

    if( input_graphics_file( surface_filename, &format, &n_surf_objects,
                             &surf_objects ) != OK ||
        n_surf_objects != 1 || get_object_type(surf_objects[0]) != POLYGONS )
    {
        print( "Surface file must contain 1 polygons object.\n" );
        return( 1 );
    }

    surface = get_polygons_ptr( surf_objects[0] );

    if( filename_extension_matches( input_filename, "obj" ) )
    {
        transforming_values = FALSE;

        if( input_graphics_file( input_filename, &format, &n_objects, &objects)
             != OK )
            return( 1 );
    }
    else
    {
        transforming_values = TRUE;

        if( open_file( input_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );

        ALLOC( values, surface->n_points );
        for_less( point, 0, surface->n_points )
        {
            if( input_real( file, &values[point] ) != OK )
            {
                print_error( "Error reading values file: %s\n", input_filename);
                return( 1 );
            }
        }

        (void) close_file( file );
    }

    check_polygons_neighbours_computed( surface );
    create_polygons_bintree( surface,
                             ROUND( (Real) surface->n_items * BINTREE_FACTOR ));

    /*--- create a unit sphere with same number of triangles as surface */

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               surface->n_items, &unit_sphere );

    fill_Point( pt, x1, y1, z1 );
    map_point_to_unit_sphere( surface, &pt, &unit_sphere, &unit_point1 );
    fill_Point( pt, x2, y2, z2 );
    map_point_to_unit_sphere( surface, &pt, &unit_sphere, &unit_point2 );

    if( !get_sphere_transform( (Real) Point_x(unit_point1),
                               (Real) Point_y(unit_point1),
                               (Real) Point_z(unit_point1),
                               u1, v1,
                               (Real) Point_x(unit_point2),
                               (Real) Point_y(unit_point2),
                               (Real) Point_z(unit_point2),
                               u2, v2, &transform ) )
    {
        print_error( "Error in sphere transform\n" );
        return( 1 );
    }

    compute_transform_inverse( &transform, &inverse );

    if( invert_flag )
    {
        check_polygons_neighbours_computed( &unit_sphere );
        create_polygons_bintree( &unit_sphere,
                                 ROUND( (Real) unit_sphere.n_items
                                         * BINTREE_FACTOR ));
    }

    if( n_bounding_points > 0 )
    {
        u_min = 0.0;
        u_max = 0.0;
        v_min = 0.0;
        v_max = 0.0;

        for_less( point, 0, n_bounding_points )
        {
            map_point_to_unit_sphere( surface, &bounding_points[point],
                                      &unit_sphere, &unit_point );
            transform_point( &transform,
                             (Real) Point_x(unit_point),
                             (Real) Point_y(unit_point),
                             (Real) Point_z(unit_point), &x, &y, &z );

            map_sphere_to_uv( x, y, z, &u, &v );

            if( point == 0 || u < u_min )
                u_min = u;
            if( point == 0 || u > u_max )
                u_max = u;
            if( point == 0 || v < v_min )
                v_min = v;
            if( point == 0 || v > v_max )
                v_max = v;
        }
    }

    print( " %g %g   %g %g\n", u_min, u_max, v_min, v_max );

#ifdef DEBUG
    for_less( i, 0, 3 )
    {
        for_less( j, 0, 4 )
        {
            print( " %15.7g", Transform_elem(transform,i,j) );
        }
        print( "\n" );
    }
#endif

    if( transforming_values )
    {
    }
    else
    {
        for_less( obj, 0, n_objects )
        {
            n_points = get_object_points( objects[obj], &points );

            initialize_progress_report( &progress, FALSE, n_points, "Mapping" );

            for_less( point, 0, n_points )
            {
                if( invert_flag )
                {
                    map_uv_to_sphere( (Real) Point_x(points[point]),
                                      (Real) Point_y(points[point]),
                                      &x, &y, &z );

                    transform_point( &inverse, x, y, z, &x, &y, &z );

                    fill_Point( unit_point, x, y, z );

                    map_unit_sphere_to_point( &unit_sphere, &unit_point,
                                              surface, &points[point] );
                }
                else
                {
                    map_point_to_unit_sphere( surface, &points[point],
                                              &unit_sphere, &unit_point );

                    transform_point( &transform,
                                     (Real) Point_x(unit_point),
                                     (Real) Point_y(unit_point),
                                     (Real) Point_z(unit_point), &x, &y, &z );

                    map_sphere_to_uv( x, y, z, &u, &v );

                    fill_Point( points[point], u, v, 0.0 );
                }

                update_progress_report( &progress, point + 1 );
            }

            terminate_progress_report( &progress );
        }

        (void) output_graphics_file( output_filename, format,
                                     n_objects, objects );
    }

    return( 0 );
}

private  BOOLEAN    get_sphere_transform(
    Real        x1,
    Real        y1,
    Real        z1,
    Real        u1,
    Real        v1,
    Real        x2,
    Real        y2,
    Real        z2,
    Real        u2,
    Real        v2,
    Transform   *transform )
{
    Transform   from, to;
    Vector      p1, p2, p1_can, p2_can;
    Vector      x_vec, y_vec, z_vec, x_vec_can, y_vec_can, z_vec_can;
    Point       centre;
    Real        x, y, z;

    fill_Vector( p1, x1, y1, z1 );
    fill_Vector( p2, x2, y2, z2 );

    map_uv_to_sphere( u1, v1, &x, &y, &z );
    fill_Vector( p1_can, x, y, z );
    map_uv_to_sphere( u2, v2, &x, &y, &z );
    fill_Vector( p2_can, x, y, z );

    CROSS_VECTORS( z_vec, p1, p2 );
    CROSS_VECTORS( y_vec, z_vec, p1 );

    CROSS_VECTORS( z_vec_can, p1_can, p2_can );
    CROSS_VECTORS( y_vec_can, z_vec_can, p1_can );

    if( null_Vector( &z_vec ) || null_Vector( &z_vec_can ) )
        return( FALSE );

    fill_Point( centre, 0.0, 0.0, 0.0 );

    NORMALIZE_VECTOR( x_vec, p1 );
    NORMALIZE_VECTOR( y_vec, y_vec );
    NORMALIZE_VECTOR( z_vec, z_vec );

    NORMALIZE_VECTOR( x_vec_can, p1_can );
    NORMALIZE_VECTOR( y_vec_can, y_vec_can );
    NORMALIZE_VECTOR( z_vec_can, z_vec_can );

    make_change_from_bases_transform( &centre, &x_vec, &y_vec, &z_vec, &from );
    make_change_to_bases_transform( &centre, &x_vec_can, &y_vec_can, &z_vec_can,
                                    &to );

    concat_transforms( transform, &from, &to );

    return( TRUE );
}
