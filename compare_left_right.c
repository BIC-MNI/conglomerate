#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR   0.5

#define  THRESHOLD   3.0

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           filename, output_filename, *filenames;
    int              i, j, ni, nj, n_objects, n_surfaces, object_index;
    int              surf, group;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons;
    Real             left_x, right_x, y, z, dist;
    Real             ***samples[2], **means[2], **variance[2], **t_stat;
    Real             sum_x, sum_xx, s, se, min_t, max_t;
    Real             sx, sy, sz, ox;
    Point            point, ray_origin, min_range, max_range, origin;
    Vector           ray_direction;
    Volume           volume;
    int              sizes[N_DIMENSIONS];
    Transform        linear;
    General_transform  transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &ni ) ||
        !get_int_argument( 0, &nj ) )
    {
        print_error( "Usage: %s output.obj  ni nj input1.obj input2.obj ... \n",
                    argv[0] );
        return( 1 );
    }

    n_surfaces = 0;

    while( get_string_argument( NULL, &filename ) )
    {
        ADD_ELEMENT_TO_ARRAY( filenames, n_surfaces, filename, 1 );
    }

    ALLOC3D( samples[0], n_surfaces, ni, nj );
    ALLOC3D( samples[1], n_surfaces, ni, nj );

    for_less( surf, 0, n_surfaces )
    {
        if( input_graphics_file( filenames[surf], &format, &n_objects,
                                 &object_list ) != OK )
        {
            print( "Couldn't read %s.\n", filename );
            return( 1 );
        }
        else if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
        {
            print( "Invalid object in file.\n" );
            return( 1 );
        }
        else
        {
            polygons = get_polygons_ptr( object_list[0] );

            create_polygons_bintree( polygons,
                                     polygons->n_items * BINTREE_FACTOR );

            if( surf == 0 )
            {
                get_range_points( polygons->n_points, polygons->points,
                                  &min_range, &max_range );
            }

            for_less( i, 0, ni )
            for_less( j, 0, nj )
            {
                y = INTERPOLATE( (Real) i / (Real) (ni-1),
                                 Point_y(min_range), Point_y(max_range) );
                z = INTERPOLATE( (Real) j / (Real) (nj-1),
                                 Point_z(min_range), Point_z(max_range) );

                fill_Point( ray_origin, Point_x(max_range) + 100.0, y, z );
                fill_Vector( ray_direction, -1.0, 0.0, 0.0 );

                if( intersect_ray_with_object( &ray_origin, &ray_direction,
                                               object_list[0], &object_index,
                                               &dist, NULL ) )
                {
                    GET_POINT_ON_RAY( point, ray_origin, ray_direction, dist );
                    right_x = Point_x( point );
                }
                else
                    right_x = 0.0;

                fill_Point( ray_origin, Point_x(min_range) - 100.0, y, z );
                fill_Vector( ray_direction, 1.0, 0.0, 0.0 );

                if( intersect_ray_with_object( &ray_origin, &ray_direction,
                                               object_list[0], &object_index,
                                               &dist, NULL ) )
                {
                    GET_POINT_ON_RAY( point, ray_origin, ray_direction, dist );
                    left_x = -Point_x( point );
                }
                else
                    left_x = 0.0;

                if( ABS(left_x) < THRESHOLD && ABS(right_x) < THRESHOLD )
                {
                    left_x = 0.0;
                    right_x = 0.0;
                }

                samples[0][surf][i][j] = left_x;
                samples[1][surf][i][j] = right_x;
            }

            print( "%d:  %s\n", surf+1, filenames[surf] );

            delete_object_list( n_objects, object_list );
        }
    }

    ALLOC2D( means[0], ni, nj );
    ALLOC2D( means[1], ni, nj );
    ALLOC2D( variance[0], ni, nj );
    ALLOC2D( variance[1], ni, nj );
    ALLOC2D( t_stat, ni, nj );

    min_t = 0.0;
    max_t = 0.0;

    for_less( group, 0, 2 )
    for_less( i, 0, ni )
    for_less( j, 0, nj )
    {
        sum_x = 0.0;
        sum_xx = 0.0;

        for_less( surf, 0, n_surfaces )
        {
            sum_x += samples[group][surf][i][j];
            sum_xx += samples[group][surf][i][j] * samples[group][surf][i][j];
        }

        means[group][i][j] = sum_x / (Real) n_surfaces;
        variance[group][i][j] = (sum_xx - sum_x * sum_x / (Real) n_surfaces) /
                               (Real) (n_surfaces-1);
    }

    for_less( i, 0, ni )
    for_less( j, 0, nj )
    {
        s = sqrt( ((Real) (n_surfaces-1) * variance[0][i][j] +
                   (Real) (n_surfaces-1) * variance[1][i][j]) /
                  (Real) (n_surfaces + n_surfaces - 2) );

        se = s * sqrt( 1.0 / (Real) n_surfaces + 1.0 / (Real) n_surfaces );

        if( se == 0.0 )
            t_stat[i][j] = 0.0;
        else
            t_stat[i][j] = (means[0][i][j] - means[1][i][j]) / se;

        if( i == 0 && j == 0 || t_stat[i][j] < min_t )
            min_t = t_stat[i][j];
        if( i == 0 && j == 0 || t_stat[i][j] > max_t )
            max_t = t_stat[i][j];
    }

    volume = create_volume( 3, XYZ_dimension_names, NC_FLOAT, FALSE, 0.0, 0.0 );
    sizes[0] = 2;
    sizes[1] = ni;
    sizes[2] = nj;
    set_volume_sizes( volume, sizes );
    alloc_volume_data( volume );

    print( "Min and max: %g %g\n", min_t, max_t );
    set_volume_voxel_range( volume, min_t, max_t );
    set_volume_real_range( volume, min_t, max_t );

    for_less( i, 0, ni )
    for_less( j, 0, nj )
    {
        set_volume_real_value( volume, 0, i, j, 0, 0, t_stat[i][j] );
        set_volume_real_value( volume, 1, i, j, 0, 0, t_stat[i][j] );
    }

    sx = (Point_x(max_range) - Point_x(min_range)) / 2.0;
    sy = (Point_y(max_range) - Point_y(min_range)) / (Real) (ni-1);
    sz = (Point_z(max_range) - Point_z(min_range)) / (Real) (nj-1);
    ox = INTERPOLATE( 0.25, Point_x(min_range), Point_x(max_range) );

    make_scale_transform( sx, sy, sz, &linear );
    fill_Point( origin, ox, Point_y(min_range), Point_z(min_range) );
    set_transform_origin( &linear, &origin );

    create_linear_transform( &transform, &linear );
    set_voxel_to_world_transform( volume, &transform );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          volume, "left-right-diff\n", NULL );

    return( 0 );
}
