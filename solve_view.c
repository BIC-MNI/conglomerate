#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  N_PARMS   6

private  void  get_random_points(
    Real     size,
    int      n_points,
    Real     (*points)[N_DIMENSIONS] );

private  void   get_view_transform(
    int         n_points,
    Real        (*points)[N_DIMENSIONS],
    Real        angle,
    Transform   *view );

private  void  get_random_orientation(
    Real    size,
    Real    parameters[] );

private  void  create_transform(
    Real       parameters[],
    Transform  *transform );

private  void  solve_view(
    int         n_points,
    Real        (*points)[N_DIMENSIONS],
    Real        (*screen_points)[2],
    Transform   *solution );

int   main(
    int    argc,
    char   *argv[] )
{
    int          p, iter, n_iters, seed, n_points, i, j, n_success;
    Real         parameters[N_PARMS], x, y, z;
    Real         angle, size, x_scale;
    Real         (*points)[N_DIMENSIONS];
    Real         (*screen_points)[2], tol;
    Transform    view, transform, solution, random_transform, scale;
    BOOLEAN      outside_view;

    initialize_argument_processing( argc, argv );

    (void) get_int_argument( 10, &n_points );
    (void) get_int_argument( 1, &n_iters );
    (void) get_real_argument( 1.0e-6, &tol );
    (void) get_real_argument( 300.0, &size );
    (void) get_real_argument( 90.0, &angle );
    (void) get_int_argument( 12392342, &seed );

    set_random_seed( seed );

    ALLOC( points, n_points );
    ALLOC( screen_points, n_points );

    n_success = 0;

    for_less( iter, 0, n_iters )
    {
        get_random_points( size, n_points, points );

        get_view_transform( n_points, points, angle, &view );

        get_random_orientation( size, parameters );

        create_transform( parameters, &transform );

        concat_transforms( &random_transform, &transform, &view );

        x_scale = Transform_elem( random_transform, 0, 0 );

        if( x_scale == 0.0 )
            print( "Found 0 x_scale[%d]\n", iter );

        make_scale_transform( 1.0 / x_scale, 1.0 / x_scale, 1.0 / x_scale,
                              &scale );

        concat_transforms( &random_transform, &random_transform, &scale );

        outside_view = FALSE;

        for_less( p, 0, n_points )
        {
            transform_point( &random_transform, points[p][X], points[p][Y],
                             points[p][Z], &x, &y, &z );

            if( z == 0 )
                handle_internal_error( "z==0" );

            screen_points[p][X] = x / z;
            screen_points[p][Y] = y / z;

/*
            print( "%d: %g %g\n", p, screen_points[p][X], screen_points[p][Y] );
*/

            if( screen_points[p][X] < -1.0 || screen_points[p][X] > 1.0 ||
                screen_points[p][Y] < -1.0 || screen_points[p][Y] > 1.0 )
            {
                outside_view = TRUE;
            }
        }

        if( outside_view )
            continue;

        ++n_success;

        solve_view( n_points, points, screen_points, &solution );

        for_less( i, 0, 4 )
        {
            for_less( j, 0, 4 )
                if( !numerically_close( Transform_elem(random_transform,i,j),
                                        Transform_elem(solution,i,j), tol ) )
                    break;
            if( j < 4 )
                break;
        }

        if( i < 4 )
        {
            for_less( i, 0, 4 )
            {
                for_less( j, 0, 4 )
                    print( " %.7g", Transform_elem(random_transform,i,j) );
                print( "\n" );
            }
            print( "\n" );

            for_less( i, 0, 4 )
            {
                for_less( j, 0, 4 )
                    print( " %.7g", Transform_elem(solution,i,j) );
                print( "\n" );
            }
            print( "\n" );
        }
    }

    print( "%d/%d\n", n_success, n_iters );

    return( 0 );
}

private  void  get_random_points(
    Real     size,
    int      n_points,
    Real     (*points)[N_DIMENSIONS] )
{
    int   p, dim;

    for_less( p, 0, n_points )
    for_less( dim, 0, N_DIMENSIONS )
        points[p][dim] = size * (2.0 * get_random_0_to_1() - 1.0);
}

private  void   get_view_transform(
    int         n_points,
    Real        (*points)[N_DIMENSIONS],
    Real        angle,
    Transform   *view )
{
    int        p, dim;
    Real       centroid[N_DIMENSIONS];
    Real       origin[N_DIMENSIONS];
    Real       min_pos[N_DIMENSIONS], max_pos[N_DIMENSIONS];
    Real       dist, z_scale, half_width;
    Transform  translation, scale;

    for_less( dim, 0, N_DIMENSIONS )
    {
        min_pos[dim] = points[0][dim];
        max_pos[dim] = points[0][dim];
        for_less( p, 0, n_points )
        {
            min_pos[dim] = MIN( min_pos[dim], points[p][dim] );
            max_pos[dim] = MAX( max_pos[dim], points[p][dim] );
        }
    }

    dist = (max_pos[X] - min_pos[X]) / tan( angle / 2.0 * DEG_TO_RAD );

    origin[X] = (min_pos[X] + max_pos[X]) / 2.0;
    origin[Y] = (min_pos[Y] + max_pos[Y]) / 2.0;
    origin[Z] = max_pos[Z] + dist;

    half_width = (max_pos[X] - min_pos[X]) / 2.0;

    make_translation_transform( -origin[X], -origin[Y], -origin[Z],
                                &translation );

    z_scale = 2.0 * half_width / -dist;

    make_scale_transform( 1.0, 1.0, z_scale, &scale );

    concat_transforms( view, &translation, &scale );
}

private  void  get_random_orientation(
    Real    size,
    Real    parameters[] )
{
    parameters[0] = get_random_0_to_1() * 2.0 * PI;
    parameters[1] = get_random_0_to_1() * 2.0 * PI;
    parameters[2] = get_random_0_to_1() * 2.0 * PI;

    parameters[3] = size / 5.0 * (2.0 * get_random_0_to_1() - 1.0);
    parameters[4] = size / 5.0 * (2.0 * get_random_0_to_1() - 1.0);
    parameters[5] = size / 5.0 * (2.0 * get_random_0_to_1() - 1.0);
}

private  void  create_transform(
    Real       parameters[],
    Transform  *transform )
{
    Transform  translation, x_rot, y_rot, z_rot;

    make_rotation_transform( parameters[0], X, &x_rot );
    make_rotation_transform( parameters[1], Y, &y_rot );
    make_rotation_transform( parameters[2], Z, &z_rot );
    make_translation_transform( parameters[3], parameters[4], parameters[5],
                                &translation );

    concat_transforms( transform, &x_rot, &y_rot );
    concat_transforms( transform, transform, &z_rot );
    concat_transforms( transform, transform, &translation );
}

private  void  solve_view(
    int         n_points,
    Real        (*points)[N_DIMENSIONS],
    Real        (*screen_points)[2],
    Transform   *solution )
{
    int                   p, dim;
    Real                  x, y, z, xs, ys, parameters[11], *weights;
    linear_least_squares  lsq;

    initialize_linear_least_squares( &lsq, 11 );

    ALLOC( weights, 11 );

    for_less( p, 0, n_points )
    {
        x = points[p][X];
        y = points[p][Y];
        z = points[p][Z];
        xs = screen_points[p][X];
        ys = screen_points[p][Y];

        for_less( dim, 0, 11 )
            weights[dim] = 0.0;

        weights[0] = y;
        weights[1] = z;
        weights[2] = 1;
        weights[7] = -xs * x;
        weights[8] = -xs * y;
        weights[9] = -xs * z;
        weights[10] = -xs;

        add_to_linear_least_squares( &lsq, weights, -x );

        for_less( dim, 0, 11 )
            weights[dim] = 0.0;

        weights[3] = x;
        weights[4] = y;
        weights[5] = z;
        weights[6] = 1;
        weights[7] = -ys * x;
        weights[8] = -ys * y;
        weights[9] = -ys * z;
        weights[10] = -ys;

        add_to_linear_least_squares( &lsq, weights, 0.0 );
    }

    FREE( weights );

    (void) get_linear_least_squares_solution( &lsq, parameters );

    delete_linear_least_squares( &lsq );

    make_identity_transform( solution );

    Transform_elem(*solution,0,0) = 1.0;
    Transform_elem(*solution,0,1) = parameters[0];
    Transform_elem(*solution,0,2) = parameters[1];
    Transform_elem(*solution,0,3) = parameters[2];

    Transform_elem(*solution,1,0) = parameters[3];
    Transform_elem(*solution,1,1) = parameters[4];
    Transform_elem(*solution,1,2) = parameters[5];
    Transform_elem(*solution,1,3) = parameters[6];

    Transform_elem(*solution,2,0) = parameters[7];
    Transform_elem(*solution,2,1) = parameters[8];
    Transform_elem(*solution,2,2) = parameters[9];
    Transform_elem(*solution,2,3) = parameters[10];
}
