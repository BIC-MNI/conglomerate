#include  <volume_io.h>
#include  <bicpl.h>
#include  <conjugate_min.h>

static  void  fit_curve(
    int     n_points,
    VIO_Point   points[],
    VIO_Real    stretch_weight,
    VIO_Real    smoothness_weight,
    int     n_cvs,
    VIO_Point   cvs[] );

static  int  get_minimum_spanning_tree_main_branch(
    int    n_points,
    VIO_Point  points[],
    int    indices[] );

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input_lines.tag  output_lines.obj n_mm_per_segment\n\
                  [stretch_weight] [smoothness_weight] [disjoint_distance]\n\
\n\
     Creates a piecewise linear curve that approximates the set of points in\n\
     the tag file.  The n_mm_per_segment sets\n\
     the number of mm length of the line segments\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               input_filename, output_filename;
    VIO_Real                 **tags, stretch_weight, smoothness_weight;
    VIO_Real                 disjoint_distance, sq_disjoint_distance;
    VIO_Real                 n_mm_per_segment, length, pos, delta_length;
    VIO_Real                 current_length;
    int                  *tree_indices, n_span, n_segments;
    int                  p, n_tag_points, n_volumes;
    int                  i, n_cvs;
    int                  n_class_points, p2, p1, n_classes;
    int                  current_ind;
    object_struct        **object_list;
    lines_struct         *lines;
    VIO_Point                *points, *class_points;
    BOOLEAN              break_up_flag;
    int                  *classes, cl, n_changed;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &n_mm_per_segment ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 1.0, &stretch_weight );
    (void) get_real_argument( 10.0, &smoothness_weight );
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

        if( n_class_points == 1 )
        {
            initialize_lines_with_size( lines, WHITE, 2, FALSE );
            lines->points[0] = class_points[0];
            lines->points[1] = class_points[0];
            continue;
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

        n_segments = VIO_ROUND( length / n_mm_per_segment );

        if( n_segments < 1 )
        {
            initialize_lines_with_size( lines, WHITE, 2, FALSE );
            get_points_centroid( n_class_points, class_points,
                                 &lines->points[0] );
            lines->points[1] = lines->points[0];
            continue;
        }

        n_cvs = n_segments + 1;
        initialize_lines_with_size( lines, WHITE, n_cvs, FALSE );

        current_ind = 0;
        current_length = 0.0;

        for_less( i, 0, n_cvs )
        {
            pos = length * ((VIO_Real) i / (VIO_Real) n_segments);
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
                lines->points[i] = class_points[tree_indices[0]];
            }
            else if( current_ind > n_span-1 )
            {
                lines->points[i] = class_points[tree_indices[n_span-1]];
            }
            else
            {
                INTERPOLATE_POINTS( lines->points[i], class_points
                                    [tree_indices[current_ind-1]],
                                    class_points[tree_indices[current_ind]],
                                    (pos - (current_length - delta_length)) /
                                    delta_length );
            }
        }

        fit_curve( n_class_points, class_points,
                   stretch_weight, smoothness_weight, n_cvs, lines->points );

        FREE( tree_indices );
    }

    if( output_graphics_file( output_filename, ASCII_FORMAT, n_classes,
                              object_list ) != OK)
        return( 1 );

    FREE( class_points );
    FREE( points );
    delete_object_list( n_classes, object_list );

    return( 0 );
}

static  void  get_minimum_spanning_tree(
    int    n_points,
    VIO_Point  points[],
    int    (*edges)[2] )
{
    VIO_Real          min_dist, dist;
    int           p, min_ind, n_edges, i, i1, i2, min_i1, min_i2;
    VIO_SCHAR  *included;

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

static  void  get_distance_from_vertex(
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

static  int  get_minimum_spanning_tree_main_branch(
    int    n_points,
    VIO_Point  points[],
    int    indices[] )
{
    int               (*tree_edges)[2], i1, i2;
    int               edge, max_dist, ind, *dists;
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

static  VIO_Real  point_segment_dist(
    VIO_Point   *point,
    VIO_Real    seg1[],
    VIO_Real    seg2[],
    VIO_Real    *alpha )
{
    VIO_Real   dx, dy, dz, len, dot_prod;

    dx = seg2[0] - seg1[0];
    dy = seg2[1] - seg1[1];
    dz = seg2[2] - seg1[2];

    len = dx * dx + dy * dy + dz * dz;

    dot_prod = (RPoint_x(*point) - seg1[0]) * dx +
               (RPoint_y(*point) - seg1[1]) * dy +
               (RPoint_z(*point) - seg1[2]) * dz;

    if( dot_prod <= 0.0 )
    {
        *alpha = 0.0;
        dx = RPoint_x(*point) - seg1[0];
        dy = RPoint_y(*point) - seg1[1];
        dz = RPoint_z(*point) - seg1[2];
    }
    else if( dot_prod >= 0.0 )
    {
        *alpha = 1.0;
        dx = RPoint_x(*point) - seg2[0];
        dy = RPoint_y(*point) - seg2[1];
        dz = RPoint_z(*point) - seg2[2];
    }
    else
    {
        *alpha = dot_prod / len;

        dx = (1.0 - *alpha) * seg1[0] + *alpha * seg2[0] - RPoint_x(*point);
        dy = (1.0 - *alpha) * seg1[1] + *alpha * seg2[1] - RPoint_y(*point);
        dz = (1.0 - *alpha) * seg1[2] + *alpha * seg2[2] - RPoint_z(*point);
    }

    return( dx * dx + dy * dy + dz * dz );
}

static  void  get_point_deriv(
    VIO_Point   *point,
    VIO_Real    seg_point[],
    VIO_Real    deriv[] )
{
    deriv[0] = 2.0 * (seg_point[0] - RPoint_x(*point));
    deriv[1] = 2.0 * (seg_point[1] - RPoint_y(*point));
    deriv[2] = 2.0 * (seg_point[2] - RPoint_z(*point));
}

static  void  get_segment_deriv(
    VIO_Point   *point,
    VIO_Real    seg1[],
    VIO_Real    seg2[],
    VIO_Real    deriv[] )
{
    int    i;
    VIO_Real   px, py, pz, s1x, s1y, s1z, s2x, s2y, s2z;
    VIO_Real   dot_top, dot_bottom, top_deriv[6], bottom_deriv[6];

    px = RPoint_x(*point);
    py = RPoint_y(*point);
    pz = RPoint_z(*point);
    s1x = seg1[0];
    s1y = seg1[1];
    s1z = seg1[2];
    s2x = seg2[0];
    s2y = seg2[1];
    s2z = seg2[2];

    dot_top = (px - s1x) * (s2x - s1x) +
              (py - s1y) * (s2y - s1y) +
              (pz - s1z) * (s2z - s1z);

    dot_bottom = (s2x - s1x) * (s2x - s1x) +
                 (s2y - s1y) * (s2y - s1y) +
                 (s2z - s1z) * (s2z - s1z);

    top_deriv[0] = 2.0 * dot_top * (-(s2x-s1x)-(px-s1x));
    top_deriv[1] = 2.0 * dot_top * (-(s2y-s1y)-(py-s1y));
    top_deriv[2] = 2.0 * dot_top * (-(s2z-s1z)-(pz-s1z));
    top_deriv[3] = 2.0 * dot_top * 2.0 * (px - s1x);
    top_deriv[4] = 2.0 * dot_top * 2.0 * (py - s1y);
    top_deriv[5] = 2.0 * dot_top * 2.0 * (pz - s1z);

    bottom_deriv[0] = -2.0 * (s2x - s1x);
    bottom_deriv[1] = -2.0 * (s2y - s1y);
    bottom_deriv[2] = -2.0 * (s2z - s1z);
    bottom_deriv[3] =  2.0 * (s2x - s1x);
    bottom_deriv[4] =  2.0 * (s2y - s1y);
    bottom_deriv[5] =  2.0 * (s2z - s1z);

    deriv[0] = -2.0 * (px - s1x);
    deriv[1] = -2.0 * (py - s1y);
    deriv[2] = -2.0 * (pz - s1z);
    deriv[3] = 0.0;
    deriv[4] = 0.0;
    deriv[5] = 0.0;

    for_less( i, 0, 6 )
    {
        deriv[i] -= top_deriv[i] / dot_bottom - dot_top * dot_top /
                       dot_bottom / dot_bottom * bottom_deriv[i];
    }
}

static  int   get_closest_segment(
    VIO_Point   *point,
    int     n_points,
    VIO_Real    parameters[],
    VIO_Real    *best_dist,
    VIO_Real    *best_alpha )
{
    VIO_Real  dist, alpha;
    int   seg, best_seg;

    *best_dist = 0.0;
    for_less( seg, 0, n_points-1 )
    {
        dist = point_segment_dist( point, &parameters[3*seg],
                                          &parameters[3*(seg+1)], &alpha );

        if( seg == 0 || dist < *best_dist )
        {
            best_seg = seg;
            *best_dist = dist;
            *best_alpha = alpha;
        }
    }

    return( best_seg );
}

static  VIO_Real  evaluate_fit(
    int    n_segments,
    VIO_Real   parameters[],
    int    n_points,
    VIO_Point  points[],
    VIO_Real   stretch_weight,
    VIO_Real   smoothness_weight )
{
    int    p, seg;
    VIO_Real   fit, dist, alpha, fit2, dx, dy, dz, fit3, cx, cy, cz;

    fit = 0.0;

    for_less( p, 0, n_points )
    {
        (void) get_closest_segment( &points[p], n_segments+1, parameters,
                                    &dist, &alpha );

        fit += dist;
    }

    fit2 = 0.0;
    for_less( seg, 0, n_segments )
    {
        dx = parameters[3*seg+0] - parameters[3*(seg+1)+0];
        dy = parameters[3*seg+1] - parameters[3*(seg+1)+1];
        dz = parameters[3*seg+2] - parameters[3*(seg+1)+2];

        fit2 += dx * dx + dy * dy + dz * dz;
    }

    fit3 = 0.0;
    for_less( seg, 0, n_segments-1 )
    {
        cx = (parameters[3*seg+0] + parameters[3*(seg+2)+0]) / 2.0;
        cy = (parameters[3*seg+1] + parameters[3*(seg+2)+1]) / 2.0;
        cz = (parameters[3*seg+2] + parameters[3*(seg+2)+2]) / 2.0;
        dx = cx - parameters[3*(seg+1)+0];
        dy = cy - parameters[3*(seg+1)+1];
        dz = cz - parameters[3*(seg+1)+2];

        fit3 += dx * dx + dy * dy + dz * dz;
    }

    return( fit + fit2 * stretch_weight + fit3 * smoothness_weight );
}

static  void  evaluate_fit_deriv(
    int    n_segments,
    VIO_Real   parameters[],
    int    n_points,
    VIO_Point  points[],
    VIO_Real   stretch_weight,
    VIO_Real   smoothness_weight,
    VIO_Real   deriv[] )
{
    int    p, seg;
    VIO_Real   dist, alpha, point_deriv[3], seg_deriv[6], dx, dy, dz, cx, cy, cz;

    for_less( p, 0, 3 * (n_segments+1) )
        deriv[p] = 0.0;

    for_less( p, 0, n_points )
    {
        seg = get_closest_segment( &points[p], n_segments+1, parameters,
                                   &dist, &alpha );

        if( alpha <= 0.0 )
        {
            get_point_deriv( &points[p], &parameters[3*seg], point_deriv );
            deriv[3*seg+0] += point_deriv[0];
            deriv[3*seg+1] += point_deriv[1];
            deriv[3*seg+2] += point_deriv[2];
        }
        else if( alpha >= 1.0 )
        {
            get_point_deriv( &points[p], &parameters[3*(seg+1)], point_deriv );
            deriv[3*(seg+1)+0] += point_deriv[0];
            deriv[3*(seg+1)+1] += point_deriv[1];
            deriv[3*(seg+1)+2] += point_deriv[2];
        }
        else
        {
            get_segment_deriv( &points[p], &parameters[3*seg],
                               &parameters[3*(seg+1)], seg_deriv );
            deriv[3*seg+0] += seg_deriv[0];
            deriv[3*seg+1] += seg_deriv[1];
            deriv[3*seg+2] += seg_deriv[2];
            deriv[3*(seg+1)+0] += seg_deriv[3];
            deriv[3*(seg+1)+1] += seg_deriv[4];
            deriv[3*(seg+1)+2] += seg_deriv[5];
        }
    }

    for_less( seg, 0, n_segments )
    {
        dx = parameters[3*seg+0] - parameters[3*(seg+1)+0];
        dy = parameters[3*seg+1] - parameters[3*(seg+1)+1];
        dz = parameters[3*seg+2] - parameters[3*(seg+1)+2];

        deriv[3*seg+0] += 2.0 * stretch_weight * dx;
        deriv[3*seg+1] += 2.0 * stretch_weight * dy;
        deriv[3*seg+2] += 2.0 * stretch_weight * dz;
        deriv[3*(seg+1)+0] += -2.0 * stretch_weight * dx;
        deriv[3*(seg+1)+1] += -2.0 * stretch_weight * dy;
        deriv[3*(seg+1)+2] += -2.0 * stretch_weight * dz;
    }

    for_less( seg, 0, n_segments-1 )
    {
        cx = (parameters[3*seg+0] + parameters[3*(seg+2)+0]) / 2.0;
        cy = (parameters[3*seg+1] + parameters[3*(seg+2)+1]) / 2.0;
        cz = (parameters[3*seg+2] + parameters[3*(seg+2)+2]) / 2.0;
        dx = cx - parameters[3*(seg+1)+0];
        dy = cy - parameters[3*(seg+1)+1];
        dz = cz - parameters[3*(seg+1)+2];

        deriv[3*seg+0] += 2.0 * smoothness_weight * dx / 2.0;
        deriv[3*seg+1] += 2.0 * smoothness_weight * dy / 2.0;
        deriv[3*seg+2] += 2.0 * smoothness_weight * dz / 2.0;
        deriv[3*(seg+2)+0] += 2.0 * smoothness_weight * dx / 2.0;
        deriv[3*(seg+2)+1] += 2.0 * smoothness_weight * dy / 2.0;
        deriv[3*(seg+2)+2] += 2.0 * smoothness_weight * dz / 2.0;
        deriv[3*(seg+1)+0] += -2.0 * smoothness_weight * dx;
        deriv[3*(seg+1)+1] += -2.0 * smoothness_weight * dy;
        deriv[3*(seg+1)+2] += -2.0 * smoothness_weight * dz;
    }
}

typedef  struct
{
    int     n_segments;
    int     n_points;
    VIO_Point   *points;
    VIO_Real    stretch_weight;
    VIO_Real    smoothness_weight;
} func_data_struct;

static  VIO_Real  function(
    VIO_Real  parameters[],
    void  *void_ptr )
{
    func_data_struct   *data;

    data = (func_data_struct *) void_ptr;

    return( evaluate_fit( data->n_segments, parameters,
                          data->n_points, data->points,
                          data->stretch_weight, data->smoothness_weight) );
}

static  void  function_deriv(
    VIO_Real  parameters[],
    void  *void_ptr,
    VIO_Real  deriv[] )
{
    func_data_struct   *data;

    data = (func_data_struct *) void_ptr;

    evaluate_fit_deriv( data->n_segments, parameters,
                        data->n_points, data->points, data->stretch_weight,
                        data->smoothness_weight, deriv );
}

static  void  fit_curve(
    int     n_points,
    VIO_Point   points[],
    VIO_Real    stretch_weight,
    VIO_Real    smoothness_weight,
    int     n_cvs,
    VIO_Point   cvs[] )
{
    int                p, n_iterations;
    VIO_Real               *parameters, range_tolerance, domain_tolerance, fit;
    VIO_Real               line_min_range_tolerance, line_min_domain_tolerance;
    func_data_struct   data;

    stretch_weight *= (VIO_Real) n_points / (VIO_Real) (n_cvs-1);
    if( n_cvs < 3 )
        smoothness_weight = 0.0;
    else
        smoothness_weight *= (VIO_Real) n_points / (VIO_Real) (n_cvs-2);

    ALLOC( parameters, 3 * n_cvs );

    for_less( p, 0, n_cvs )
    {
        parameters[3*p+0] = RPoint_x(cvs[p]);
        parameters[3*p+1] = RPoint_y(cvs[p]);
        parameters[3*p+2] = RPoint_z(cvs[p]);
    }

    range_tolerance = 1.0e-3;
    domain_tolerance = 1.0e-3;
    line_min_range_tolerance = 1.0e-3;
    line_min_domain_tolerance = 1.0e-3;
    n_iterations = 300;

    data.n_segments = n_cvs-1;
    data.n_points = n_points;
    data.points = points;
    data.stretch_weight = stretch_weight;
    data.smoothness_weight = smoothness_weight;

    fit = conjugate_minimize_function( 3 * n_cvs, parameters,
                                       function, function_deriv,
                                       (void *) &data, range_tolerance,
                                       domain_tolerance,
                                       line_min_range_tolerance,
                                       line_min_domain_tolerance,
                                       n_iterations, 2,
                                       parameters );

    for_less( p, 0, n_cvs )
    {
        fill_Point( cvs[p], parameters[3*p+0],
                            parameters[3*p+1],
                            parameters[3*p+2] );
    }

    FREE( parameters );
}
