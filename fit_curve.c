#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  get_best_line(
    int     n_points,
    Point   points[],
    Point   *line_origin,
    Vector  *line_direction,
    int     *best_axis );

private  void  fit_curve(
    int     n_points,
    Point   points[],
    Point   *line_origin,
    Vector  *line_direction,
    int     axis,
    Real    smoothness_weight,
    int     n_cvs,
    Point   cvs[] );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input_lines.tag  output_lines.obj n_controls n_intervals\n\
                  [smoothness_weight]\n\
\n\
     Creates a cubic spline curve that approximates the set of points in\n\
     the tag file.  The n_controls effectively sets the smoothness of the \n\
     spline by controling the number of control vertices.  The n_intervals\n\
     is the number of intervals between each control vertex in the\n\
     discretized line that is finally output.  The optional smoothness_weight\n\
     sets the amount of smoothing of the spline\n\
     (0 = no smoothing, .001 = reasonable smoothing).\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename;
    Real                 **tags, u, smoothness_weight;
    int                  p, n_piecewise, n_tag_points, n_volumes, axis;
    int                  n_intervals_per, i, n_cvs, ind, n;
    object_struct        *object;
    lines_struct         *lines;
    Point                *points, line_origin, *cvs;
    Vector               line_direction;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &n_piecewise ) ||
        !get_int_argument( 0, &n_intervals_per ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &smoothness_weight );

    if( input_tag_file( input_filename, &n_volumes, &n_tag_points,
                        &tags, NULL, NULL, NULL, NULL, NULL ) != OK )
        return( 1 );

    if( n_tag_points == 0 )
    {
        if( output_graphics_file( output_filename, ASCII_FORMAT, 0, NULL ) !=OK)
            return( 1 );
        return( 0 );
    }
    else if( n_tag_points == 1 )
    {
        object = create_object( LINES );
        lines = get_lines_ptr( object );
        initialize_lines_with_size( lines, WHITE, 1, FALSE );
        fill_Point( lines->points[0], tags[0][X], tags[0][Y], tags[0][Z] );
        if( output_graphics_file( output_filename, ASCII_FORMAT, 1, &object )
                                  !=OK )
            return( 1 );
        return( 0 );
    }

    ALLOC( points, n_tag_points );

    for_less( p, 0, n_tag_points )
        fill_Point( points[p], tags[p][X], tags[p][Y], tags[p][Z] );

    get_best_line( n_tag_points, points, &line_origin, &line_direction, &axis );

    n_cvs = MAX( n_piecewise + 2, 5 );

    ALLOC( cvs, n_cvs );

    fit_curve( n_tag_points, points, &line_origin, &line_direction, axis,
               smoothness_weight, n_cvs, cvs );

    object = create_object( LINES );
    lines = get_lines_ptr( object );
    initialize_lines_with_size( lines, WHITE, n_intervals_per * (n_cvs-3) + 1,
                                FALSE );

    ind = 0;
    for_less( p, 1, n_cvs-2 )
    {
        if( p == n_cvs-3 )
            n = n_intervals_per + 1;
        else
            n = n_intervals_per;
        for_less( i, 0, n )
        {
            u = (Real) i / (Real) (n_intervals_per-1);
            fill_Point( lines->points[ind],
                        cubic_interpolate( u, RPoint_x(cvs[p-1]),
                                              RPoint_x(cvs[p+0]),
                                              RPoint_x(cvs[p+1]),
                                              RPoint_x(cvs[p+2]) ),
                        cubic_interpolate( u, RPoint_y(cvs[p-1]),
                                              RPoint_y(cvs[p+0]),
                                              RPoint_y(cvs[p+1]),
                                              RPoint_y(cvs[p+2]) ),
                        cubic_interpolate( u, RPoint_z(cvs[p-1]),
                                              RPoint_z(cvs[p+0]),
                                              RPoint_z(cvs[p+1]),
                                              RPoint_z(cvs[p+2]) ) );
            ++ind;
        }
    }

    if( ind != lines->n_points )
        handle_internal_error( " ind != lines->n_points" );

    if( output_graphics_file( output_filename, ASCII_FORMAT, 1, &object ) != OK)
        return( 1 );

    return( 0 );
}

private  BOOLEAN  get_best_line_using_axis_2d(
    int     n_points,
    Point   points[],
    int     axis,
    Real    vy,
    Real    vz,
    Point   *line_origin,
    Vector  *line_direction,
    Real    *error )
{
    int                    a1, a2, p;
    Real                   p_dot_p, x, y, z, p_dot_v;
    Real                   second_deriv, constant;
    Point                  centroid;
    Vector                 offset;
    BOOLEAN                okay;

    get_points_centroid( n_points, points, &centroid );

    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    second_deriv = 0.0;
    constant = 0.0;

    for_less( p, 0, n_points )
    {
        SUB_POINTS( offset, points[p], centroid );
        p_dot_p = DOT_VECTORS( offset, offset );
        x = RVector_coord( offset, axis );
        y = RVector_coord( offset, a1 );
        z = RVector_coord( offset, a2 );

        second_deriv += 2.0 * p_dot_p - 2.0 * x * x;
        constant += -2.0 * x * (y*vy+z*vz);
    }

    okay = (second_deriv != 0.0);

    if( okay )
    {
        *line_origin = centroid;
        Vector_coord( *line_direction, axis ) = (Point_coord_type)
                                                (-constant / second_deriv);
        Vector_coord( *line_direction, a1 ) = (Point_coord_type) vy;
        Vector_coord( *line_direction, a2 ) = (Point_coord_type) vz;

        *error = 0.0;
        for_less( p, 0, n_points )
        {
            SUB_POINTS( offset, points[p], centroid );
            p_dot_v = DOT_VECTORS( offset, *line_direction );
            *error += DOT_VECTORS( offset, offset ) *
                      DOT_VECTORS( *line_direction, *line_direction ) -
                      p_dot_v * p_dot_v;
        }
    }

    return( okay );
}

private  BOOLEAN  get_best_line_using_axis(
    int     n_points,
    Point   points[],
    int     axis,
    Point   *line_origin,
    Vector  *line_direction,
    Real    *error )
{
    int                    a1, a2, p;
    Real                   p_dot_p, x, y, z, p_dot_v;
    Real                   **second_derivs, solution[2], constants[2];
    Point                  centroid;
    Vector                 offset;
    BOOLEAN                okay;

    get_points_centroid( n_points, points, &centroid );

    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    ALLOC2D( second_derivs, 2, 2 );
    second_derivs[0][0] = 0.0;
    second_derivs[0][1] = 0.0;
    second_derivs[1][0] = 0.0;
    second_derivs[1][1] = 0.0;
    constants[0] = 0.0;
    constants[1] = 0.0;

    for_less( p, 0, n_points )
    {
        SUB_POINTS( offset, points[p], centroid );
        p_dot_p = DOT_VECTORS( offset, offset );
        x = RVector_coord( offset, a1 );
        y = RVector_coord( offset, a2 );
        z = RVector_coord( offset, axis );

        second_derivs[0][0] += 2.0 * p_dot_p - 2.0 * x * x;
        second_derivs[0][1] += -2.0 * x * y;
        constants[0] -= -2.0 * x * z * 1.0;
        second_derivs[1][1] += 2.0 * p_dot_p - 2.0 * y * y;
        second_derivs[1][0] += -2.0 * x * y;
        constants[1] -= -2.0 * y * z * 1.0;
    }

    okay = solve_linear_system( 2, second_derivs, constants, solution );

    FREE2D( second_derivs );

    if( okay )
    {
        *line_origin = centroid;
        Vector_coord( *line_direction, axis ) = (Point_coord_type) 1.0;
        Vector_coord( *line_direction, a1 ) = (Point_coord_type) solution[0];
        Vector_coord( *line_direction, a2 ) = (Point_coord_type) solution[1];

        *error = 0.0;
        for_less( p, 0, n_points )
        {
            SUB_POINTS( offset, points[p], centroid );
            p_dot_v = DOT_VECTORS( offset, *line_direction );
            *error += DOT_VECTORS( offset, offset ) *
                      DOT_VECTORS( *line_direction, *line_direction ) -
                      p_dot_v * p_dot_v;
        }
    }

    return( okay );
}

private  void  get_best_line(
    int     n_points,
    Point   points[],
    Point   *line_origin,
    Vector  *line_direction,
    int     *best_axis )
{
    int      axis, value;
    BOOLEAN  found;
    Real     best_error, error;
    Point    origin;
    Vector   direction;

    found = FALSE;
    best_error = 0.0;

    for_less( axis, 0, N_DIMENSIONS )
    for_less( value, 0, 2 )
    {
        if( get_best_line_using_axis_2d( n_points, points, axis,
                                         (Real) value, 1.0 - (Real) value,
                                         &origin, &direction, &error ) )
        {
            if( !found || error < best_error )
            {
                found = TRUE;
                best_error = error;
                *line_origin = origin;
                *line_direction = direction;
                if( value == 0 )
                    *best_axis = (axis + 1) % N_DIMENSIONS;
                else
                    *best_axis = (axis + 2) % N_DIMENSIONS;
            }
        }
    }

    if( !found )
        handle_internal_error( "get_best_line" );
}

private  void  get_spline_fit(
    int      n_points,
    Point    points[],
    Real     smoothness_weight,
    int      n_cvs,
    Point    cvs[] )
{
    int                    p, i, off, b;
    linear_least_squares   lsq;
    Real                   **basis, *coefs, u, x, y, weight, power, h;

    ALLOC2D( basis, 4, 4 );

    get_cubic_spline_coefs( basis );

    ALLOC( coefs, n_cvs-2 );

    initialize_linear_least_squares( &lsq, n_cvs - 2 );

    for_less( p, 0, n_points )
    {
        x = RPoint_x(points[p]);
        y = RPoint_y(points[p]);
        i = 1;
        while( x > RPoint_x(cvs[i+1]) )
        {
            ++i;
        }
        if( i > n_cvs - 3 || x < RPoint_x(cvs[i]) )
            handle_internal_error( "get_spline_fit" );

        for_less( b, 0, n_cvs-2 )
            coefs[b] = 0.0;

        u = (x - RPoint_x(cvs[i])) / (RPoint_x(cvs[i+1])-RPoint_x(cvs[i]));

        for_less( off, -1, 3 )
        {
            weight = 0.0;
            power = 1.0;
            for_less( b, 0, 4 )
            {
                weight += basis[b][off+1] * power;
                power *= u;
            }

            if( i + off == 0 )
            {
                coefs[0] +=  2.5 * weight;
                coefs[1] += -2.0 * weight;
                coefs[2] +=  0.5 * weight;
            }
            else if( i + off == n_cvs-1 )
            {
                coefs[n_cvs-5] +=  0.5 * weight;
                coefs[n_cvs-4] += -2.0 * weight;
                coefs[n_cvs-3] +=  2.5 * weight;
            }
            else
            {
                coefs[i+off-1] += weight;
            }
        }

        add_to_linear_least_squares( &lsq, coefs, y );
    }

    h = 1.0 / (Real) (n_cvs-3);
    smoothness_weight *= sqrt( (Real) n_points / (Real) (n_cvs-4) ) / h / h;

    for_less( p, 1, n_cvs-3 )
    {
        for_less( b, 0, n_cvs-2 )
            coefs[b] = 0.0;

        coefs[p-1] = 1.0 * smoothness_weight;
        coefs[p+0] = -2.0 * smoothness_weight;
        coefs[p+1] = 1.0 * smoothness_weight;

        add_to_linear_least_squares( &lsq, coefs, 0.0 );
    }

    if( !get_linear_least_squares_solution( &lsq, coefs ) )
        handle_internal_error( "get_spline_fit" );

    for_less( p, 1, n_cvs-1 )
    {
        Point_y(cvs[p]) = (Point_coord_type) coefs[p-1];
    }

    Point_y(cvs[0]) = (Point_coord_type)
                      (2.5 * coefs[0] - 2.0 * coefs[1] + 0.5 * coefs[2]);
    Point_y(cvs[n_cvs-1]) = (Point_coord_type)
                      (2.5 * coefs[n_cvs-3] - 2.0 * coefs[n_cvs-4] +
                       0.5 * coefs[n_cvs-5]);

    delete_linear_least_squares( &lsq );

    FREE( coefs );
    FREE2D( basis );
}

private  void  fit_curve(
    int     n_points,
    Point   points[],
    Point   *line_origin,
    Vector  *line_direction,
    int     axis,
    Real    smoothness_weight,
    int     n_cvs,
    Point   cvs[] )
{
    int     p;
    Real    t_min, t_max, t, x, y;
    Vector  offset, vert, plane;
    Point   *aligned_points;

    ALLOC( aligned_points, n_points );
    fill_Vector( plane, 0.0, 0.0, 0.0 );
    Vector_coord(plane,axis) = (Point_coord_type) 1.0;

    CROSS_VECTORS( vert, plane, *line_direction );
    NORMALIZE_VECTOR( vert, vert );

    t_min = 0.0;
    t_max = 0.0;

    for_less( p, 0, n_points )
    {
        SUB_POINTS( offset, points[p], *line_origin );
        t = DOT_VECTORS( offset, *line_direction ) /
            DOT_VECTORS( *line_direction, *line_direction );

        if( p == 0 || t < t_min )
            t_min = t;

        if( p == 0 || t > t_max )
            t_max = t;

        x = t;
        y = DOT_VECTORS( offset, vert );

        fill_Point( aligned_points[p], x, y, 0.0 );
    }

    for_less( p, 0, n_cvs )
    {
        x = INTERPOLATE( (Real) (p-1) / (Real) (n_cvs-3), t_min, t_max );
        fill_Point( cvs[p], x, 0.0, 0.0 );
    }

    get_spline_fit( n_points, aligned_points, smoothness_weight, n_cvs, cvs );

    for_less( p, 0, n_cvs )
    {
        x = RPoint_x(cvs[p]);
        y = RPoint_y(cvs[p]);

        GET_POINT_ON_RAY( cvs[p], *line_origin, *line_direction, x);
        GET_POINT_ON_RAY( cvs[p], cvs[p], vert, y );
    }

    FREE( aligned_points );
}
