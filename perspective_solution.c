#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>
#include  <stdlib.h>

#define  N_PARAMS   12

private  void  transform_to_screen(
    Transform   *t,
    Point       *world_point,
    Real        *u,
    Real        *v );

private  BOOLEAN  compute_perspective_transform(
    int        n_points,
    Point      screen_points[],
    Point      world[],
    int        n_iters,
    Transform  *transform );

private  BOOLEAN  solve_minimization_without_dimension(
    Real   seconds_derivs[N_PARAMS][N_PARAMS],
    int    dim_to_leave_out,
    Real   solution[] );

private  void  generate_points(
    int    n_points,
    Point  world_points[] );

private  void  generate_transform(
    Transform   *transform );

private  void  generate_screen_points(
    int         n_points,
    Point       world_points[],
    Transform   *transform,
    Real        error_size,
    Point       screen_points[] );

private  void  print_transform(
    char       title[],
    Transform  *transform );

private  BOOLEAN  transforms_close(
    Transform  *t1,
    Transform  *t2,
    Real       tolerance );

private  void  normalize_transform(
    Transform  *t );

int  main(
    int   argc,
    char  *argv[] )
{
    FILE        *file;
    BOOLEAN     file_present;
    char        *points_filename;
    int         n_iters, n_points, i, n_tries, n_correct, u, v;
    Real        tolerance, error, x, y, z, test_u, test_v;
    Transform   true_transform, test_transform;
    Point       *screen_points, *world_points;

    initialize_argument_processing( argc, argv );

    srand48( 8392851 );

    (void) get_int_argument( 100, &n_iters );
    (void) get_int_argument( 1, &n_tries );
    (void) get_real_argument( 0.0, &error );
    (void) get_int_argument( 20, &n_points );
    (void) get_real_argument( 1.0e-3, &tolerance );

    if( get_string_argument( "", &points_filename ) )
    {
        file_present = TRUE;
        if( open_file( points_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );

        (void) input_int( file, &n_points );

        ALLOC( screen_points, n_points );
        ALLOC( world_points, n_points );

        for_less( i, 0, n_points )
        {
            (void) input_int( file, &u );
            (void) input_int( file, &v );

            fill_Point( screen_points[i], (Real) u, (Real) v, 0.0 );
        }

        for_less( i, 0, n_points )
        {
            (void) input_real( file, &x );
            (void) input_real( file, &y );
            (void) input_real( file, &z );

            fill_Point( world_points[i], x, y, z );
        }

        close_file( file );

        n_iters = 1;
    }
    else
    {
        file_present = FALSE;

        ALLOC( screen_points, n_points );
        ALLOC( world_points, n_points );
    }

    n_correct = 0;

    for_less( i, 0, n_iters )
    {
        if( !file_present )
        {
            generate_points( n_points, world_points );
            generate_transform( &true_transform );
            generate_screen_points( n_points, world_points, &true_transform,
                                    error, screen_points );
        }

        if( !compute_perspective_transform( n_points, screen_points,
                                            world_points, n_tries,
                                            &test_transform ) )
        {
            print( "Could not compute transform.\n" );
            return( 1 );
        }

        if( file_present )
        {
            print_transform( "Solution: ", &test_transform );
        }
        else
        {
            normalize_transform( &test_transform );
            normalize_transform( &true_transform );

            if( !transforms_close( &test_transform, &true_transform, tolerance))
            {
                print( "\n------------------- error --------------------\n" );
                print_transform( "True: ", &true_transform );
                print_transform( "Test: ", &test_transform );
            }
            else
                ++n_correct;

#ifdef DEBUG
            for_less( i, 0, n_points )
            {
                transform_to_screen( &test_transform, &world_points[i],
                                     &test_u, &test_v );

                print( "%2d: %g %g  === %g %g\n",
                       i,
                       Point_x(screen_points[i]),
                       Point_y(screen_points[i]),
                       test_u, test_v );
            }
#endif
        }
    }

    if( !file_present )
    {
        if( n_correct == n_iters )
            print( "All %d transformations correctly solved.\n", n_iters );
        else
            print( "Solved %d out of %d transformations correctly.\n",
                   n_correct, n_iters );
    }
    else
    {
        for_less( i, 0, n_points )
        {
            transform_to_screen( &test_transform, &world_points[i],
                                 &test_u, &test_v );

            print( "%2d: %g %g  === %g %g   world %g %g %g\n",
                   i,
                   Point_x(screen_points[i]),
                   Point_y(screen_points[i]),
                   test_u, test_v,
                   Point_x(world_points[i]),
                   Point_y(world_points[i]),
                   Point_z(world_points[i]) );
        }
    }

    FREE( screen_points );
    FREE( world_points );

    return( 0 );
}

private  BOOLEAN  compute_weighted_perspective_transform(
    int        n_points,
    Point      screen_points[],
    Point      world[],
    Real       weights[],
    Transform  *transform )
{
    int       i, j, p, d;
    Real      second_deriv_coefs[N_PARAMS][N_PARAMS];
    Real      set_of_coefs[N_PARAMS];
    Real      parameters[N_PARAMS], best_parameters[N_PARAMS];
    Real      max, best_max;
    BOOLEAN   found;

    for_less( i, 0, N_PARAMS )
        for_less( j, 0, N_PARAMS )
            second_deriv_coefs[i][j] = 0.0;

    for_less( p, 0, n_points )
    {
        for_less( d, 0, 2 )
        {
            for_less( i, 0, 3 )
            {
                set_of_coefs[8+i] = Point_coord(screen_points[p],d) *
                                    Point_coord(world[p],i);
            }

            set_of_coefs[8+3] = Point_coord(screen_points[p],d);

            for_less( i, 0, 3 )
                set_of_coefs[4*d+i] = -Point_coord(world[p],i);
            set_of_coefs[4*d+3] = -1.0;

            for_less( i, 0, 4 )
                set_of_coefs[4*(1-d)+i] = 0.0;

            for_less( i, 0, N_PARAMS )
                for_less( j, i, N_PARAMS )
                    second_deriv_coefs[i][j] += weights[p] * 2.0 *
                                                set_of_coefs[i] *
                                                set_of_coefs[j];
        }
    }

    for_less( i, 0, N_PARAMS-1 )
        for_less( j, i+1, N_PARAMS )
            second_deriv_coefs[j][i] = second_deriv_coefs[i][j];

    found = FALSE;
    best_max = 0.0;

    for_less( i, 0, N_PARAMS )
    {
        if( solve_minimization_without_dimension( second_deriv_coefs, i,
                                                  parameters ) )
        {
            max = 0.0;
            for_less( j, 0, N_PARAMS )
            {
                if( j == 0 || ABS(parameters[j]) > max )
                    max = ABS(parameters[j]);
            }

            if( !found || max < best_max )
            {
                found = TRUE;
                best_max = max;
                for_less( j, 0, N_PARAMS )
                    best_parameters[j] = parameters[j];

                if( best_max == 1.0 )
                    break;
            }
        }
    }

    if( found )
    {
        for_less( i, 0, 3 )
            for_less( j, 0, 4 )
                Transform_elem(*transform,i,j) = best_parameters[IJ(i,j,4)];

        Transform_elem(*transform,3,0) = 0.0;
        Transform_elem(*transform,3,1) = 0.0;
        Transform_elem(*transform,3,2) = 0.0;
        Transform_elem(*transform,3,3) = 1.0;
    }

    return( found );
}


private  BOOLEAN  compute_perspective_transform(
    int        n_points,
    Point      screen_points[],
    Point      world[],
    int        n_iters,
    Transform  *transform )
{
    int       p, iter;
    Real      *weights, x, y, z;
    BOOLEAN   found;

    ALLOC( weights, n_points );
    for_less( p, 0, n_points )
        weights[p] = 1.0;

    for_less( iter, 0, n_iters )
    {
        found = compute_weighted_perspective_transform( n_points,
                      screen_points, world, weights, transform );

        if( !found )
            break;

        for_less( p, 0, n_points )
        {
            transform_point( transform, Point_x(world[p]), Point_y(world[p]),
                             Point_z(world[p]), &x, &y, &z );

            if( z == 0.0 )
                handle_internal_error( "compute_perspective_transform" );

            weights[p] = 1.0 / z / z;
        }
    }

    FREE( weights );

    return( found );
}

private  BOOLEAN  solve_minimization_without_dimension(
    Real   seconds_derivs[N_PARAMS][N_PARAMS],
    int    dim_to_leave_out,
    Real   solution[] )
{
    BOOLEAN  found;
    int      i, j, i_ind, j_ind;
    Real     **mat, constants[N_PARAMS-1], value_to_set;
    Real     answer[N_PARAMS-1];

    value_to_set = 1.0;

    for_less( i, 0, N_PARAMS-1 )
    {
        i_ind = ((i < dim_to_leave_out) ? i : i + 1 );
        constants[i] = -seconds_derivs[i_ind][dim_to_leave_out] * value_to_set;
    }

    ALLOC2D( mat, N_PARAMS-1, N_PARAMS-1 );

    for_less( i, 0, N_PARAMS-1 )
    {
        i_ind = ((i < dim_to_leave_out) ? i : i + 1 );
        for_less( j, 0, N_PARAMS-1 )
        {
            j_ind = ((j < dim_to_leave_out) ? j : j + 1 );

            mat[i][j] = seconds_derivs[i_ind][j_ind];
        }
    }

    found = solve_linear_system( N_PARAMS-1, mat, constants, answer );

    if( found )
    {
        for_less( i, 0, N_PARAMS-1 )
        {
            i_ind = ((i < dim_to_leave_out) ? i : i + 1 );
            solution[i_ind] = answer[i];
        }

        solution[dim_to_leave_out] = value_to_set;
    }

    FREE2D( mat );

    return( found );
}

#define  MIN_BOX  -10.0
#define  MAX_BOX   10.0

private  void  generate_point(
    Real   min_box,
    Real   max_box,
    Point  *point )
{
    int   c;

    for_less( c, 0, N_DIMENSIONS )
    {
        Point_coord(*point,c) = min_box + (max_box - min_box) *
                                drand48();
    }
}

private  void  generate_points(
    int    n_points,
    Point  world_points[] )
{
    int   i;

    for_less( i, 0, n_points )
        generate_point( MIN_BOX, MAX_BOX, &world_points[i] );
}

private  void  generate_transform(
    Transform   *transform )
{
    BOOLEAN     in_box;
    int         c;
    Real        tx, ty, tz;
    Point       eye, look_at, origin;
    Vector      x_axis, z_axis;
    Transform   scale_transform;

    generate_point( MIN_BOX, MAX_BOX, &look_at );

    do
    {
        generate_point( 10.0 * MIN_BOX, 10.0 * MAX_BOX, &eye );

        in_box = TRUE;
        for_less( c, 0, N_DIMENSIONS )
        {
            if( Point_coord(eye,c) < MIN_BOX || Point_coord(eye,c) > MAX_BOX )
            {
                in_box = FALSE;
                break;
            }
        }
    }
    while( in_box );

    SUB_POINTS( z_axis, eye, look_at );

    do
    {
        for_less( c, 0, N_DIMENSIONS )
            Vector_coord( x_axis, c ) = 2.0 * drand48() - 1.0;
    }
    while( DOT_VECTORS( x_axis, z_axis ) == 0.0 );

    make_identity_transform( transform );

    set_transform_x_and_z_axes( transform, &x_axis, &z_axis );

    transform_point( transform, Point_x(eye), Point_y(eye), Point_z(eye),
                     &tx, &ty, &tz );

    fill_Point( origin, -tx, -ty, -tz );

    set_transform_origin( transform, &origin );

    make_scale_transform( 100000.0, 100000.0, 1.0 + drand48() * 10.0,
                          &scale_transform );

    concat_transforms( transform, transform, &scale_transform );
}

private  void  generate_screen_points(
    int         n_points,
    Point       world_points[],
    Transform   *transform,
    Real        error_size,
    Point       screen_points[] )
{
    int   i;
    Real  u, v, w, x_error, y_error, z_error;

    for_less( i, 0, n_points )
    {
        x_error = error_size * (2.0 * drand48() - 1.0);
        y_error = error_size * (2.0 * drand48() - 1.0);
        z_error = error_size * (2.0 * drand48() - 1.0);

        transform_point( transform,
                         Point_x(world_points[i]) + x_error,
                         Point_y(world_points[i]) + y_error,
                         Point_z(world_points[i]) + z_error,
                         &u, &v, &w );

        if( w == 0.0 )
            handle_internal_error( "generate_screen_points" );

        fill_Point( screen_points[i], u / w, v / w, 0.0 );
    }
}

private  void  print_transform(
    char       title[],
    Transform  *transform )
{
    int   i, j;

    print( "%s\n", title );
    for_less( i, 0, 4 )
    {
        for_less( j, 0, 4 )
        {
            print( " %18g", Transform_elem(*transform,i,j) );
        }
        print( "\n" );
    }
}

private  BOOLEAN  transforms_close(
    Transform  *t1,
    Transform  *t2,
    Real       tolerance )
{
    int   i, j;

    for_less( i, 0, 4 )
    {
        for_less( j, 0, 4 )
        {
            if( !numerically_close( Transform_elem(*t1,i,j),
                                    Transform_elem(*t2,i,j),
                                    tolerance ) )
                return( FALSE );
        }
    }

    return( TRUE );
}

private  void  normalize_transform(
    Transform  *t )
{
    Real  max_value;
    int   i, j;

    max_value = 0.0;
    for_less( i, 0, 4 )
    {
        for_less( j, 0, 4 )
            if( i == 0 && j == 0 ||
                ABS(Transform_elem(*t,i,j)) > ABS(max_value) )
                max_value = Transform_elem(*t,i,j);
    }

    if( max_value == 0.0 )
    {
        print( "Zero transforms.\n" );
        return;
    }

/*
    max_value = Transform_elem( *t, 3, 3 );
*/

    for_less( i, 0, 3 )
        for_less( j, 0, 4 )
            Transform_elem(*t,i,j) /= max_value;
}

private  void  transform_to_screen(
    Transform   *t,
    Point       *world_point,
    Real        *u,
    Real        *v )
{
    Real  x, y, z;

    transform_point( t, Point_x(*world_point),
                     Point_y(*world_point),
                     Point_z(*world_point), &x, &y, &z );

    *u = x / z;
    *v = y / z;
}
