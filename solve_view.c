#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <interval.h>

extern       int swapRM(int x);

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

private  void  solve_view_by_least_squares(
    int         n_points,
    Real        (*points)[N_DIMENSIONS],
    Real        (*screen_points)[2],
    Transform   *solution );

private  void  solve_view_by_intervals(
    int         n_points,
    Real        (*points)[N_DIMENSIONS],
    Real        (*screen_points)[2],
    Real        tolerance,
    Transform   *solution );

static  Transform r_t;

int   main(
    int    argc,
    char   *argv[] )
{
    int          p, iter, n_iters, seed, n_points, i, j, n_success;
    Real         parameters[N_PARMS], x, y, z;
    Real         angle, size, x_scale, x_pos, y_pos;
    Real         (*points)[N_DIMENSIONS];
    Real         (*screen_points)[2], tol, int_tolerance;
    Real         x_pixel_size, y_pixel_size;
    Transform    view, transform, solution, scale, random_transform;
    Transform    solution2;
    BOOLEAN      outside_view;

    initialize_argument_processing( argc, argv );

    (void) get_int_argument( 10, &n_points );
    (void) get_int_argument( 1, &n_iters );
    (void) get_real_argument( 256.0, &x_pixel_size );
    (void) get_real_argument( 256.0, &y_pixel_size );
    (void) get_real_argument( 1.0e-6, &tol );
    (void) get_real_argument( 1.0e-6, &int_tolerance );
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

r_t = random_transform;

        outside_view = FALSE;

        for_less( p, 0, n_points )
        {
            transform_point( &random_transform, points[p][X], points[p][Y],
                             points[p][Z], &x, &y, &z );

            if( z == 0.0 )
                handle_internal_error( "z==0" );

            screen_points[p][X] = x / z;
            screen_points[p][Y] = y / z;

            x_pos = (int) (x_pixel_size * (screen_points[p][X] + 1.0) / 2.0);
            screen_points[p][X] = -1.0 + 2.0 * ((Real) x_pos + 0.5) / x_pixel_size;
            y_pos = (int) (y_pixel_size * (screen_points[p][Y] + 1.0) / 2.0);
            screen_points[p][Y] = -1.0 + 2.0 * ((Real) y_pos + 0.5) / y_pixel_size;

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

        solve_view_by_least_squares( n_points, points, screen_points,
                                     &solution );

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

#ifdef NOT_YET
        solve_view_by_intervals( n_points, points, screen_points,
                                 int_tolerance, &solution2 );

        for_less( i, 0, 4 )
        {
            for_less( j, 0, 4 )
                if( !numerically_close( Transform_elem(random_transform,i,j),
                                        Transform_elem(solution2,i,j), tol ) )
                    break;
            if( j < 4 )
                break;
        }

        if( i < 4 )
        {
            print( "Error in solution2\n\n" );

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
                    print( " %.7g", Transform_elem(solution2,i,j) );
                print( "\n" );
            }
            print( "\n" );
        }
#endif
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

private  void  solve_view_by_least_squares(
    int         n_points,
    Real        (*points)[N_DIMENSIONS],
    Real        (*screen_points)[2],
    Transform   *solution )
{
    int                   p, dim;
    Real                  x, y, z, xs, ys, parameters[11], weights[11];
    linear_least_squares  lsq;

    initialize_linear_least_squares( &lsq, 11 );

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
        weights[2] = 1.0;
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
        weights[6] = 1.0;
        weights[7] = -ys * x;
        weights[8] = -ys * y;
        weights[9] = -ys * z;
        weights[10] = -ys;

        add_to_linear_least_squares( &lsq, weights, 0.0 );
    }

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

typedef  enum  { SOLUTION, NOT_SOLUTION, MIXED_SOLUTION, DIV_BY_ZERO }
               Solution_types;

private  Solution_types  check_parameters(
    int         n_points,
    Interval    (*points)[N_DIMENSIONS],
    Interval    (*screen_points)[2],
    Interval    parameters[] )
{
    int        dim, p;
    Interval   trans[N_DIMENSIONS], t1, t2, t3, x, xs, ys, cx, cy, xz, yz;
    Real       sl, sh, tl, th;
    Solution_types   sol_type;

    sol_type = SOLUTION;

    for_less( p, 0, n_points )
    {
        for_less( dim, 0, N_DIMENSIONS )
        {
            MULT_INTERVALS( t1, points[p][X], parameters[4*dim+0] );
            MULT_INTERVALS( t2, points[p][Y], parameters[4*dim+1] );
            MULT_INTERVALS( t3, points[p][Z], parameters[4*dim+2] );

            ADD_INTERVALS( x, parameters[4*dim+3], t1 );
            ADD_INTERVALS( x, x, t2 );
            ADD_INTERVALS( trans[dim], x, t3 );
        }

        MULT_INTERVALS( xz, screen_points[p][0], trans[2] );
        MULT_INTERVALS( yz, screen_points[p][1], trans[2] );

        SUBTRACT_INTERVALS( cx, trans[0], xz );
        SUBTRACT_INTERVALS( cy, trans[1], yz );

        if( !INTERVAL_CONTAINS( cx, 0.0 ) || !INTERVAL_CONTAINS( cy, 0.0 ) )
        {
            return( NOT_SOLUTION );
        }

        if( INTERVAL_SIZE( cx ) != 0.0 || INTERVAL_SIZE( cy ) != 0.0 )
            sol_type = MIXED_SOLUTION;
    }

    return( sol_type );
}

private  BOOLEAN  interval_solve_view(
    int         n_points,
    Interval    (*points)[N_DIMENSIONS],
    Interval    (*screen_points)[2],
    Interval    parameters[],
    Real        tolerance,
    Interval    solution[] )
{
    int             p, largest_index;
    Real            size, largest_size, mid;
    Interval        save_int;
    Solution_types  situation;

    situation = check_parameters( n_points, points, screen_points, parameters );

    if( situation == SOLUTION )
    {
        for_less( p, 0, 12 )
            solution[p] = parameters[p];

        return( TRUE );
    }
    else if( situation == NOT_SOLUTION )
    {
        return( FALSE );
    }
    else
    {
        largest_index = -1;
        largest_size = 0.0;

        for_less( p, 0, 12 )
        {
            size = INTERVAL_SIZE( parameters[p] );
            if( largest_index < 0 || size > largest_size )
            {
                largest_size = size;
                largest_index = p;
            }
        }

        if( largest_size <= tolerance )
        {
            if( situation == DIV_BY_ZERO )
                return( FALSE );

            for_less( p, 0, 12 )
                solution[p] = parameters[p];
    
            return( TRUE );
        }

        save_int = parameters[largest_index];
        mid = INTERVAL_MIDPOINT( parameters[largest_index] );

        SET_INTERVAL( parameters[largest_index], INTERVAL_MIN(save_int), mid );

        if( interval_solve_view( n_points, points, screen_points, parameters,
                                 tolerance, solution ) )
            return( TRUE );

        SET_INTERVAL( parameters[largest_index], mid, INTERVAL_MAX(save_int) );

        if( interval_solve_view( n_points, points, screen_points, parameters,
                                 tolerance, solution ) )
            return( TRUE );

        parameters[largest_index] = save_int;
    }

    return( FALSE );
}

private  void  prune_outer_intervals(
    int         n_points,
    Interval    (*points)[N_DIMENSIONS],
    Interval    (*screen_points)[2],
    Interval    parameters[],
    Real        tolerance )
{
    int       p, iters;
    BOOLEAN   changed;
    Real      size, edge;
    Interval  save_param;
    Solution_types  situation;

    for_less( p, 0, 12 )
    {
        print( " [ %g %g ]", INTERVAL_MIN(parameters[p]),
                             INTERVAL_MAX(parameters[p]) );
    }
    print( "\n" );

    iters = 0;
    changed = TRUE;
    while( changed )
    {
        changed = FALSE;

        for_less( p, 0, 12 )
        {
            save_param = parameters[p];
            size = INTERVAL_SIZE( parameters[p] );
            while( size > tolerance )
            {
                size /= 2.0;

                edge = INTERVAL_MIN(save_param) + size;

                SET_INTERVAL( parameters[p], INTERVAL_MIN(save_param), edge );

                situation = check_parameters( n_points, points, screen_points,
                                              parameters );

                if( situation == NOT_SOLUTION )
                {
                    SET_INTERVAL( parameters[p],
                                  edge, INTERVAL_MAX(save_param) );
                    save_param = parameters[p];
                    changed = TRUE;
                }
                else
                    parameters[p] = save_param;
            }

            save_param = parameters[p];
            size = INTERVAL_SIZE( parameters[p] );
            while( size > tolerance )
            {
                size /= 2.0;

                edge = INTERVAL_MAX(save_param) - size;

                SET_INTERVAL( parameters[p], edge, INTERVAL_MAX(save_param) );

                situation = check_parameters( n_points, points, screen_points,
                                              parameters );

                if( situation == NOT_SOLUTION )
                {
                    SET_INTERVAL( parameters[p],
                                  INTERVAL_MIN(save_param), edge );
                    save_param = parameters[p];
                    changed = TRUE;
                }
                else
                    parameters[p] = save_param;
            }
        }

        ++iters;
    }
    while( changed );

    if( iters > 1 )
    {
        for_less( p, 0, 12 )
        {
            print( " [ %g %g ]", INTERVAL_MIN(parameters[p]),
                                 INTERVAL_MAX(parameters[p]) );
        }
        print( "\n" );
    }
}

private  void  solve_view_by_intervals(
    int         n_points,
    Real        (*points)[N_DIMENSIONS],
    Real        (*screen_points)[2],
    Real        tolerance,
    Transform   *solution )
{
    int        p, i, j;
    Interval   (*int_points)[N_DIMENSIONS], (*int_screen_points)[2];
    Interval   initial_guess[12], answer[12];
    Real       x_scale;
    Transform  scale;

    ALLOC( int_points, n_points );
    ALLOC( int_screen_points, n_points );

    for_less( p, 0, n_points )
    {
        SET_INTERVAL( int_points[p][0], points[p][0], points[p][0] );
        SET_INTERVAL( int_points[p][1], points[p][1], points[p][1] );
        SET_INTERVAL( int_points[p][2], points[p][2], points[p][2] );
        SET_INTERVAL( int_screen_points[p][0], screen_points[p][0],
                                               screen_points[p][0] );
        SET_INTERVAL( int_screen_points[p][1], screen_points[p][1],
                                               screen_points[p][1] );
    }

    for_less( p, 0, 12 )
    {
        SET_INTERVAL( initial_guess[p], -1.0e30, 1.0e30 );
{
    Real  value;

    value = Transform_elem( r_t, p / 4, p % 4 );
    if( value < 0.0 )
    {
        SET_INTERVAL( initial_guess[p], value * 1.01, value * 0.99 );
    }
    else
    {
        SET_INTERVAL( initial_guess[p], value * 0.99, value * 1.01 );
    }
}
    }

    SET_INTERVAL( initial_guess[0], 1.0, 1.0 );

    prune_outer_intervals( n_points, int_points, int_screen_points,
                           initial_guess, tolerance );

    if( !interval_solve_view( n_points, int_points, int_screen_points,
                              initial_guess, tolerance, answer ) )
    {
        handle_internal_error( "solve_view_by_intervals" );
    }

    for_less( i, 0, 3 )
    {
        for_less( j, 0, 4 )
        {
            print( "   %g +/- %g", INTERVAL_MIDPOINT(answer[IJ(i,j,4)]),
                                   INTERVAL_SIZE(answer[IJ(i,j,4)]) / 2.0 );
        }
        print( "\n" );
    }

    FREE( int_points );
    FREE( int_screen_points );

    make_identity_transform( solution );

    for_less( i, 0, 3 )
    for_less( j, 0, 4 )
    {
        Transform_elem( *solution, i,j) = INTERVAL_MIDPOINT(answer[IJ(i,j,4)]);
    }

    x_scale = Transform_elem( *solution, 0, 0 );

    if( x_scale == 0.0 )
        print( "Found 0 x_scale\n" );

    make_scale_transform( 1.0 / x_scale, 1.0 / x_scale, 1.0 / x_scale, &scale );

    concat_transforms( solution, solution, &scale );
}
