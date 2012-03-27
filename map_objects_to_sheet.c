#include  <volume_io.h>
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
                       [x_size y_size] \n\
                       [u_min u_max v_min v_max] | \n\
                       [bound_file.obj] \n\
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
    int                  n_objects, obj, n_points, x_size, y_size;
    int                  n_surf_objects, n_args;
    int                  n_bounding_points, point, vertex, size, poly;
    FILE                 *file;
    Transform            transform, inverse;
    Point                *bounding_points, *points;
    Point                poly_point, polygon_vertices[MAX_POINTS_PER_POLYGON];
    Real                 weights[MAX_POINTS_PER_POLYGON];
    Real                 x, y, z, min_value, max_value, value;
    Real                 u, v, u1, v1, u2, v2, u_min, u_max, v_min, v_max;
    Real                 x1, y1, z1, x2, y2, z2, *values;
    Real                 bottom_left[N_DIMENSIONS], zero[N_DIMENSIONS];
    Real                 voxel[N_DIMENSIONS];
    Real                 separations[N_DIMENSIONS];
    int                  x_voxel, y_voxel, n_bound_objects;
    object_struct        **objects, **surf_objects;
    Point                unit_point, centre, unit_point1, unit_point2, pt;
    progress_struct      progress;
    BOOLEAN              invert_flag, transforming_values;
    int                  sizes[N_DIMENSIONS];
    STRING               dim_names[] = { MIzspace, MIxspace, MIyspace };
    STRING               bound_filename;
    object_struct        **bound_objects;
    Volume               image;

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

    (void) get_real_argument( 0.0, &x1 );
    (void) get_real_argument( 0.0, &y1 );
    (void) get_real_argument( 0.0, &z1 );
    (void) get_real_argument( 0.0, &u1 );
    (void) get_real_argument( 0.0, &v1 );
    (void) get_real_argument( 0.0, &x2 );
    (void) get_real_argument( 0.0, &y2 );
    (void) get_real_argument( 0.0, &z2 );
    (void) get_real_argument( 0.0, &u2 );
    (void) get_real_argument( 0.0, &v2 );

    if( filename_extension_matches( input_filename, "obj" ) )
        transforming_values = FALSE;
    else
        transforming_values = TRUE;

    if( transforming_values )
    {
        if( !get_int_argument( 0, &x_size ) ||
            !get_int_argument( 0, &y_size ) )
        {
            usage( argv[0] );
            return( 1 );
        }

        n_args = get_n_arguments_remaining();

        if( n_args == 0 )
        {
            u_min = 0.0;
            u_max = 1.0;
            v_min = 0.0;
            v_max = 1.0;
        }
        else if( n_args == 4 )
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
        else if( n_args == 1 )
        {
            if( !get_string_argument( NULL, &bound_filename ) ||
                input_graphics_file( bound_filename, &format,
                                     &n_bound_objects,
                                     &bound_objects ) != OK )
            {
                usage( argv[0] );
                return( 1 );
            }

            n_bounding_points = 0;
            bounding_points = NULL;

            for_less( obj, 0, n_bound_objects )
            {
                n_points = get_object_points( bound_objects[obj], &points );

                for_less( point, 0, n_points )
                {
                    ADD_ELEMENT_TO_ARRAY( bounding_points, n_bounding_points,
                                          points[point], DEFAULT_CHUNK_SIZE );
                }
            }

            delete_object_list( n_bound_objects, bound_objects );
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

    if( !transforming_values )
    {
        if( input_graphics_file( input_filename, &format, &n_objects, &objects)
             != OK )
            return( 1 );
    }
    else
    {
        if( open_file( input_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );

        min_value = 0.0;
        max_value = 0.0;
        ALLOC( values, surface->n_points );
        for_less( point, 0, surface->n_points )
        {
            if( input_real( file, &values[point] ) != OK )
            {
                print_error( "Error reading values file: %s\n", input_filename);
                return( 1 );
            }

            if( point == 0 || values[point] < min_value )
                min_value = values[point];
            if( point == 0 || values[point] > max_value )
                max_value = values[point];
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
        print( "Using identity for sphere transform.\n" );
        make_identity_transform( &transform );
    }

    compute_transform_inverse( &transform, &inverse );

    if( invert_flag || transforming_values )
    {
        check_polygons_neighbours_computed( &unit_sphere );
        create_polygons_bintree( &unit_sphere,
                                 ROUND( (Real) unit_sphere.n_items
                                         * BINTREE_FACTOR ));
    }

    if( transforming_values && n_bounding_points > 0 )
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

    if( transforming_values )
    {
        print( "U range: %g %g   V range: %g %g\n",
               u_min, u_max, v_min, v_max );
    }

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
        image = create_volume( 3, dim_names, NC_SHORT, FALSE, 0.0, 0.0 );

        sizes[0] = 2;
        sizes[1] = MAX( 2, x_size );
        sizes[2] = MAX( 2, y_size );

        set_volume_sizes( image, sizes );

        set_volume_real_range( image, min_value, max_value );

        separations[1] = (u_max - u_min) / (Real) x_size;
        separations[2] = (v_max - v_min) / (Real) y_size;
        separations[0] = (separations[1] + separations[2]) / 2.0;
        set_volume_separations( image, separations );

        bottom_left[0] = 0.5;
        bottom_left[1] = -0.5;
        bottom_left[2] = -0.5;
        zero[X] = u_min;
        zero[Y] = v_min;
        zero[Z] = 0.0;
        set_volume_translation( image, bottom_left, zero );

        alloc_volume_data( image );

        initialize_progress_report( &progress, FALSE, x_size, "Mapping" );

        for_less( x_voxel, 0, x_size )
        {
            for_less( y_voxel, 0, y_size )
            {
                voxel[0] = 0.0;
                voxel[1] = (Real) x_voxel;
                voxel[2] = (Real) y_voxel;

                convert_voxel_to_world( image, voxel, &u, &v, &z );

                map_uv_to_sphere( u, v, &x, &y, &z );

                transform_point( &inverse, x, y, z, &x, &y, &z );

                fill_Point( unit_point, x, y, z );

                poly = find_closest_polygon_point( &unit_point, &unit_sphere,
                                                   &poly_point );

                size = get_polygon_points( &unit_sphere, poly,
                                           polygon_vertices );

                get_polygon_interpolation_weights( &poly_point,
                                                   size, polygon_vertices,
                                                   weights );

                value = 0.0;

                for_less( vertex, 0, size )
                {
                    point = unit_sphere.indices[
                              POINT_INDEX(unit_sphere.end_indices,poly,vertex)];
                    value += weights[vertex] * values[point];
                }

                set_volume_real_value( image, 0, x_voxel, y_voxel, 0, 0, value);
                set_volume_real_value( image, 1, x_voxel, y_voxel, 0, 0, value);
            }

            update_progress_report( &progress, x_voxel + 1 );
        }

        if( output_volume( output_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                           image, "Flattened surface image map.\n", NULL )
                                                                   != OK )
            return( 1 );
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
