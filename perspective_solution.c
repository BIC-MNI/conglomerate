#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  N_PARAMS   12

private  BOOLEAN  compute_perspective_transform(
    int        n_points,
    Point      screen_points[],
    Point      world[],
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
    int         n_iters, n_points, i;
    Real        tolerance;
    Transform   true_transform, test_transform;
    Point       *screen_points, *world_points;

    initialize_argument_processing( argc, argv );

    set_random_seed( 8392851 );

    (void) get_int_argument( 100, &n_iters );
    (void) get_int_argument( 20, &n_points );
    (void) get_real_argument( 1.0e-3, &tolerance );

    ALLOC( screen_points, n_points );
    ALLOC( world_points, n_points );

    for_less( i, 0, n_iters )
    {
        generate_points( n_points, world_points );
        generate_transform( &true_transform );
        generate_screen_points( n_points, world_points, &true_transform,
                                screen_points );

        if( !compute_perspective_transform( n_points, screen_points,
                                            world_points, &test_transform ) )
        {
            print( "Could not compute transform.\n" );
            return( 1 );
        }

        normalize_transform( &true_transform );
        normalize_transform( &test_transform );

        if( !transforms_close( &test_transform, &true_transform, tolerance ) )
        {
            print( "\n------------------- error --------------------\n" );
            print_transform( "True: ", &true_transform );
            print_transform( "Test: ", &test_transform );
        }
    }

    FREE( screen_points );
    FREE( world_points );

    return( 0 );
}

private  BOOLEAN  compute_perspective_transform(
    int        n_points,
    Point      screen_points[],
    Point      world[],
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
                    second_deriv_coefs[i][j] += 2.0 * set_of_coefs[i] *
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
                                get_random_0_to_1();
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
    BOOLEAN   in_box;
    int       c;
    Real      tx, ty, tz;
    Point     eye, look_at, origin;
    Vector    x_axis, z_axis;

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
            Vector_coord( x_axis, c ) = 2.0 * get_random_0_to_1() - 1.0;
    }
    while( DOT_VECTORS( x_axis, z_axis ) == 0.0 );

    make_identity_transform( transform );

    set_transform_x_and_z_axes( transform, &x_axis, &z_axis );

    transform_point( transform, Point_x(eye), Point_y(eye), Point_z(eye),
                     &tx, &ty, &tz );

    fill_Point( origin, -tx, -ty, -tz );

    set_transform_origin( transform, &origin );
}

private  void  generate_screen_points(
    int         n_points,
    Point       world_points[],
    Transform   *transform,
    Point       screen_points[] )
{
    int   i;
    Real  u, v, w;

    for_less( i, 0, n_points )
    {
        transform_point( transform, Point_x(world_points[i]),
                         Point_y(world_points[i]), Point_z(world_points[i]),
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
            print( "\t%g", Transform_elem(*transform,i,j) );
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
