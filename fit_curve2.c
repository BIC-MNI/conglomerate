#include  <internal_volume_io.h>
#include  <bicpl.h>

private  int  get_minimum_spanning_tree_main_branch(
    int    n_points,
    Point  points[],
    int    indices[] );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input_lines.tag  output_lines.obj n_segments_per_mm\n\
                  [smoothness_weight] [disjoint_distance]\n\
\n\
     Creates a piecewise linear curve that approximates the set of points in\n\
     the tag file.  The n_segments_per_mm sets\n\
     the number of line segments per unit distance\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename;
    Real                 **tags, u, smoothness_weight;
    Real                 disjoint_distance, sq_disjoint_distance;
    Real                 n_segments_per_mm, length, pos, delta_length;
    Real                 current_length;
    int                  *tree_indices, n_span, n_segments;
    int                  p, n_piecewise, n_tag_points, n_volumes, axis;
    int                  n_intervals_per, i, n_cvs, ind, n;
    int                  n_class_points, p2, p1, n_classes;
    int                  current_ind;
    object_struct        **object_list;
    lines_struct         *lines;
    Point                *points, line_origin, *cvs, *class_points;
    Vector               line_direction;
    BOOLEAN              break_up_flag;
    int                  *classes, cl, n_changed;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &n_segments_per_mm ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &smoothness_weight );
    break_up_flag = get_real_argument( 0.0, &disjoint_distance );

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
        ALLOC( object_list, 1 );
        object_list[0] = create_object( LINES );
        lines = get_lines_ptr( object_list[0] );
        initialize_lines_with_size( lines, WHITE, 2, FALSE );
        fill_Point( lines->points[0], tags[0][X], tags[0][Y], tags[0][Z] );
        fill_Point( lines->points[1], tags[0][X], tags[0][Y], tags[0][Z] );
        if( output_graphics_file( output_filename, ASCII_FORMAT, 1, object_list)
                                  !=OK )
            return( 1 );

        delete_object_list( 1, object_list );
        return( 0 );
    }

    ALLOC( points, n_tag_points );
    ALLOC( classes, n_tag_points );

    for_less( p, 0, n_tag_points )
    {
        fill_Point( points[p], tags[p][X], tags[p][Y], tags[p][Z] );
        classes[p] = -1;
    }

    sq_disjoint_distance = disjoint_distance * disjoint_distance;
    n_classes = 0;

    for_less( p, 0, n_tag_points )
    {
        if( classes[p] != -1 )
            continue;

        classes[p] = n_classes;

        do
        {
            n_changed = 0;

            for_less( p1, 0, n_tag_points )
            {
                if( classes[p1] != n_classes )
                    continue;
                for_less( p2, 0, n_tag_points )
                {
                    if( classes[p2] == -1 &&
                        (!break_up_flag ||
                         sq_distance_between_points( &points[p1],
                                        &points[p2] ) <= sq_disjoint_distance) )
                    {
                        classes[p2] = n_classes;
                        ++n_changed;
                    }
                }
            }
        }
        while( n_changed > 0 );

        ++n_classes;
    }

    print( "Created %d sets of points\n", n_classes );

    ALLOC( object_list, n_classes );
    ALLOC( class_points, n_tag_points );

    for_less( cl, 0, n_classes )
    {
        object_list[cl] = create_object( LINES );
        lines = get_lines_ptr( object_list[cl] );

        n_class_points = 0;
        for_less( p, 0, n_tag_points )
        {
            if( classes[p] == cl )
            {
                class_points[n_class_points] = points[p];
                ++n_class_points;
            }
        }

        ALLOC( tree_indices, n_class_points );

        n_span = get_minimum_spanning_tree_main_branch( n_class_points,
                                                        class_points,
                                                        tree_indices );

        initialize_lines_with_size( lines, WHITE, n_span, FALSE );

        for_less( p, 0, n_span )
            lines->points[p] = class_points[tree_indices[p]];

        length = 0.0;
        for_less( p, 0, n_span-1 )
        {
            length += distance_between_points(&class_points[tree_indices[p]],
                                              &class_points[tree_indices[p+1]]);
        }

        n_segments = ROUND( n_segments_per_mm * length );

        if( n_segments < 1 )
        {
            initialize_lines_with_size( lines, WHITE, 2, FALSE );
            get_points_centroid( n_class_points, class_points,
                                 &lines->points[0] );
            lines->points[1] = lines->points[0];
            continue;
        }

        n_cvs = n_segments + 1;
        ALLOC( cvs, n_cvs );
        current_ind = 0;
        current_length = 0.0;

        for_less( i, 0, n_cvs )
        {
            pos = length * ((Real) i / (Real) n_segments);
            while( current_length < pos )
            {
                delta_length = distance_between_points(&class_points
                                   [tree_indices[current_ind]],
                                   &class_points[tree_indices[current_ind+1]]);
                current_length += delta_length;
                ++current_ind;
            }

            if( current_ind == 0 )
            {
                cvs[i] = class_points[tree_indices[0]];
            }
            else if( current_ind > n_span-1 )
            {
                cvs[i] = class_points[tree_indices[n_span-1]];
            }
            else
            {
                INTERPOLATE_POINTS( cvs[i], class_points
                                    [tree_indices[current_ind-1]],
                                    class_points[tree_indices[current_ind]],
                                    (pos - (current_length - delta_length)) /
                                    delta_length );
            }
        }

        initialize_lines_with_size( lines, WHITE, n_cvs, FALSE );

        for_less( p, 0, n_cvs )
            lines->points[p] = cvs[p];
/*

        get_best_line( n_class_points, class_points,
                       &line_origin, &line_direction, &axis );

        n_cvs = MAX( n_piecewise + 2, 5 );

        fit_curve( n_class_points, class_points,
                   &line_origin, &line_direction, axis,
                   smoothness_weight, n_cvs, cvs );

        FREE( tree_indices );

        initialize_lines_with_size( lines, WHITE,
                                    n_intervals_per * (n_cvs-3) + 1,
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
*/
        FREE( cvs );
    }

    if( output_graphics_file( output_filename, ASCII_FORMAT, n_classes,
                              object_list ) != OK)
        return( 1 );

    FREE( class_points );
    FREE( points );
    delete_object_list( n_classes, object_list );

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
    Real                   len;

    ALLOC2D( basis, 4, 4 );

    get_cubic_spline_coefs( basis );

    ALLOC( coefs, n_cvs-2 );

    len = RPoint_x(points[n_points-1]) - RPoint_x(points[0]);

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

    h = len / (Real) (n_cvs-3);
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

private  void  get_minimum_spanning_tree(
    int    n_points,
    Point  points[],
    int    (*edges)[2] )
{
    Real          min_dist, dist;
    int           p, min_ind, n_edges, i, i1, i2, min_i1, min_i2;
    Smallest_int  *included;

    ALLOC( included, n_points );

    for_less( p, 0, n_points )
    {
        included[p] = FALSE;
    }

    min_dist = sq_distance_between_points( &points[0], &points[1] );
    min_ind = 1;

    for_less( p, 2, n_points )
    {
        dist = sq_distance_between_points( &points[0], &points[p] );
        if( dist < min_dist )
        {
            min_dist = dist;
            min_ind = p;
        }
    }

    n_edges = 0;

    edges[n_edges][0] = 0;
    edges[n_edges][1] = min_ind;
    ++n_edges;
    included[0] = TRUE;
    included[min_ind] = TRUE;

    for_less( i, 0, n_points-2 )
    {
        min_i1 = -1;
        min_i2 = -1;
        for_less( i1, 0, n_points )
        {
            if( !included[i1] )
                continue;

            for_less( i2, 0, n_points )
            {
                if( included[i2] )
                    continue;

                dist = sq_distance_between_points( &points[i1], &points[i2] );
                if( min_i1 < 0 || dist < min_dist )
                {
                    min_dist = dist;
                    min_i1 = i1;
                    min_i2 = i2;
                }
            }
        }

        edges[n_edges][0] = min_i1;
        edges[n_edges][1] = min_i2;
        ++n_edges;
        included[min_i2] = TRUE;
    }

    FREE( included );

    if( n_edges != n_points-1 )
        handle_internal_error( "get_minimum_spanning_tree" );

#ifdef DEBUG
{
    object_struct  *object;
    lines_struct  *lines;
    object = create_object( LINES );
    lines = get_lines_ptr( object );

    initialize_lines( lines, WHITE );

    lines->n_points = n_points;
    lines->points = points;

    lines->n_items = n_points-1;
    ALLOC( lines->end_indices, lines->n_items );
    for_less( p, 0, lines->n_items )
        lines->end_indices[p] = 2 * (p+1);

    ALLOC( lines->indices, 2 * (n_points-1) );
    for_less( p, 0, n_points-1 )
    {
        lines->indices[2*p+0] = edges[p][0];
        lines->indices[2*p+1] = edges[p][1];
    }

    if( output_graphics_file( "debug.obj", ASCII_FORMAT, 1, &object ) != OK)
        return;
}
#endif

}

private  void  get_distance_from_vertex(
    int    n_points,
    int    (*edges)[2],
    int    vertex,
    int    distances[] )
{
    BOOLEAN           changed;
    int               i, i1, i2, edge;

    for_less( i, 0, n_points )
        distances[i] = 0;

    do
    {
        changed = FALSE;

        for_less( edge, 0, n_points-1 )
        {
            i1 = edges[edge][0];
            i2 = edges[edge][1];

            if( i1 == vertex )
            {
                if( distances[i2] != 1 )
                {
                    distances[i2] = 1;
                    changed = TRUE;
                }
            }
            else if( i2 == vertex )
            {
                if( distances[i1] != 1 )
                {
                    distances[i1] = 1;
                    changed = TRUE;
                }
            }
            else if( distances[i2] != 0 &&
                     (distances[i1] == 0 || distances[i2]+1 < distances[i1]) )
            {
                distances[i1] = distances[i2] + 1;
                changed = TRUE;
            }

            if( distances[i1] != 0 &&
                (distances[i2] == 0 || distances[i1]+1 < distances[i2]) )
            {
                distances[i2] = distances[i1] + 1;
                changed = TRUE;
            }
        }
    }
    while( changed );
}

private  int  get_minimum_spanning_tree_main_branch(
    int    n_points,
    Point  points[],
    int    indices[] )
{
    BOOLEAN           changed;
    int               (*tree_edges)[2], i1, i2, i3, dist1, dist2, test_dist;
    int               edge, max_dist, max_i1, max_i2, ind, *dists;
    int               end1, end2;

    ALLOC( tree_edges, n_points-1 );

    get_minimum_spanning_tree( n_points, points, tree_edges );

    ALLOC( dists, n_points );

    get_distance_from_vertex( n_points, tree_edges, 0, dists );

    end1 = 0;
    for_less( i1, 0, n_points )
    {
        if( dists[i1] > dists[end1] )
            end1 = i1;
    }

    get_distance_from_vertex( n_points, tree_edges, end1, dists );

    end2 = 0;
    for_less( i1, 0, n_points )
    {
        if( dists[i1] > dists[end2] )
            end2 = i1;
    }

    max_dist = dists[end2];

    indices[0] = end2;
    indices[max_dist] = end1;

    for_less( ind, 1, max_dist )
    {
        for_less( edge, 0, n_points-1 )
        {
            i1 = tree_edges[edge][0];
            i2 = tree_edges[edge][1];

            if( i1 == indices[ind-1] && dists[i2] == max_dist - ind )
            {
                indices[ind] = i2;
                break;
            }
            else if( i2 == indices[ind-1] && dists[i1] == max_dist - ind )
            {
                indices[ind] = i1;
                break;
            }
        }

        if( edge >= n_points-1 )
            handle_internal_error( "edge" );
    }

    FREE( dists );
    FREE( tree_edges );

    return( max_dist+1 );
}
