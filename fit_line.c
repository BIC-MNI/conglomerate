#include  <def_mni.h>

private  int  create_points(
    int            n_objects,
    object_struct  *object_list[],
    Point          *points[] );
private  void   fit_line_to_points(
    int            n_points,
    Point          points[],
    int            max_points,
    Boolean        subdivide,
    lines_struct   *lines );
private  int  create_curve_approximating_points(
    int            n_tags,
    Point          tags[],
    int            max_points,
    Boolean        subdivide,
    Point          *curve_points[] );
private  Real  fit_curve(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[] );
private  Boolean  should_step(
    Real   delta_energy,
    Real   temperature );
private  Real  evaluate_energy(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[],
    Real    tag_rms[],
    int     nearest_segments[],
    Real    segment_lengths[] );
private  Real   rms_error_of_point(
    Point   *point,
    int     n_points,
    Point   curve[],
    int     *nearest_segment );
private  Real  rms_to_segment(
    Point    *point,
    Point    *p1,
    Point    *p2 );
private  Real  evaluate_delta_energy(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[],
    Real    tag_rms[],
    int     nearest_segments[],
    Real    segment_lengths[],
    int     index_changed,
    int     *n_tag_changes,
    int     changed_tag[],
    Real    changed_tag_rms[],
    int     new_nearest_segments[],
    Real    new_segment_lengths[2] );

#define  DEFAULT_MAX_POINTS  9

int   main(
    int   argc,
    char  *argv[] )
{
    Status         status;
    char           *input_filename, *output_filename, *dummy;
    int            n_objects, n_points, max_points;
    Point          *points;
    Boolean        subdivide;
    object_struct  **object_list, *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        (void) fprintf( stderr, "Require input and output filename.\n" );
        return( 1 );
    }

    (void) get_int_argument( DEFAULT_MAX_POINTS, &max_points );

    subdivide = get_string_argument( "", &dummy );

    status = input_landmark_file( (Volume) NULL, input_filename,
                                  WHITE, 1.0, BOX_MARKER,
                                  &n_objects, &object_list );

    if( status != OK )
        return( 1 );

    n_points = create_points( n_objects, object_list, &points );

    delete_object_list( n_objects, object_list );

    object = create_object( LINES );

    fit_line_to_points( n_points, points, max_points, subdivide,
                        get_lines_ptr(object) );

    status = output_graphics_file( output_filename, ASCII_FORMAT,
                                   1, &object );

    return( status != OK );
}

private  int  create_points(
    int            n_objects,
    object_struct  *object_list[],
    Point          *points[] )
{
    int            i, n_points;
    marker_struct  *marker;

    n_points = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            ADD_ELEMENT_TO_ARRAY( *points, n_points, marker->position,
                                  DEFAULT_CHUNK_SIZE );
        }
    }

    return( n_points );
}

private  void   fit_line_to_points(
    int            n_points,
    Point          points[],
    int            max_points,
    Boolean        subdivide,
    lines_struct   *lines )
{
    int    i, n_curve_points;
    Point  *curve_points;

    n_curve_points = create_curve_approximating_points( n_points, points,
                                                   max_points, subdivide,
                                                   &curve_points );

    initialize_lines( lines, WHITE );

    for_less( i, 0, n_curve_points )
        add_point_to_line( lines, &curve_points[i] );
}

#define  MAX_ERROR     3.0

private  int  create_curve_approximating_points(
    int            n_tags,
    Point          tags[],
    int            max_points,
    Boolean        subdivide,
    Point          *curve_points[] )
{
    int      i, c, n_points;
    Boolean  first_time;
    Point    *curve, *tmp;
    Real     rms_error;

    n_points = 2;
    ALLOC( curve, 2 );

    curve[0] = tags[0];
    curve[1] = tags[1];

    for_less( i, 1, n_tags )
    {
        for_less( c, 0, N_DIMENSIONS )
        {
            if( Point_coord(tags[i],c) < Point_coord(curve[0],c) )
                Point_coord(curve[0],c) = Point_coord(tags[i],c);
            else if( Point_coord(tags[i],c) > Point_coord(curve[1],c) )
                Point_coord(curve[1],c) = Point_coord(tags[i],c);
        }
    }

    if( !subdivide )
    {
        REALLOC( curve, max_points );
        curve[max_points-1] = curve[1];

        for_less( i, 1, max_points-1 )
        {
            Real  ratio;

            ratio = (Real) i / (Real) (max_points-1);
            INTERPOLATE_POINTS( curve[i], curve[0], curve[max_points-1],
                                ratio );
        }
    }

    rms_error = 2.0 * MAX_ERROR;
    first_time = TRUE;

    do
    {
        if( !first_time )
        {
            ALLOC( tmp, 2 * n_points - 1 );

            for_less( i, 0, n_points )
            {
                tmp[2*i] = curve[i];
                if( i < n_points-1 )
                {
                    INTERPOLATE_POINTS( tmp[2*i+1], curve[i], curve[i+1], 0.5 );
                }
            }

            FREE( curve );
            curve = tmp;
            n_points = 2 * n_points - 1;
        }

        first_time = FALSE;

        rms_error = fit_curve( n_tags, tags, n_points, curve );
    }
    while( n_points < max_points && rms_error >= MAX_ERROR );

    *curve_points = curve;

    return( n_points );
}

#define  INITIAL_TEMPERATURE       0.01
#define  TEMPERATURE_FACTOR        0.9
#define  NUM_TEMPERATURE_STEPS     50
#define  MAX_TRIES_FACTOR          100
#define  MAX_SUCCESSES_FACTOR      10
#define  CURVE_LENGTH_FACTOR       0.1

#define  STOP_CRITERIA             3

#define  RANDOM_MOVEMENT_DISTANCE  0.5

private  Real  fit_curve(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[] )
{
    int    n_no_moves;
    Real   energy, new_energy, temperature, random_distance, rms_error;
    Real   delta_energy, min_delta_energy, max_delta_energy;
    Real   *tag_rms, *changed_tag_rms, *segment_lengths;
    Real   new_segment_lengths[2];
    Point  save_point;
    int    temperature_step, try, point_index, n_successes, max_tries;
    int    max_successes, n_pos_successes, n_tag_changes;
    int    *changed_tag, change, *nearest_segments, *new_nearest_segments;

    ALLOC( tag_rms, n_tags );
    ALLOC( changed_tag_rms, n_tags );
    ALLOC( changed_tag, n_tags );
    ALLOC( nearest_segments, n_tags );
    ALLOC( new_nearest_segments, n_tags );
    ALLOC( segment_lengths, n_points-1 );

    max_tries = MAX_TRIES_FACTOR * n_points;
    max_successes = MAX_SUCCESSES_FACTOR * n_points;

    temperature = INITIAL_TEMPERATURE;

    min_delta_energy = 0.0;
    max_delta_energy = 0.0;

    energy = evaluate_energy( n_tags, tags, n_points, curve,
                              tag_rms, nearest_segments, segment_lengths );

    print( "Starting energy: %g\n", energy );

    n_no_moves = 0;

    for_less( temperature_step, 0, NUM_TEMPERATURE_STEPS )
    {
        n_successes = 0;
        n_pos_successes = 0;

        random_distance = RANDOM_MOVEMENT_DISTANCE;

        for_less( try, 0, max_tries )
        {
            point_index = get_random_int( n_points );

            save_point = curve[point_index];

            Point_x(curve[point_index]) += random_distance *
                                           (2.0*get_random_0_to_1()-1.0);
            Point_y(curve[point_index]) += random_distance *
                                           (2.0*get_random_0_to_1()-1.0);
            Point_z(curve[point_index]) += random_distance *
                                           (2.0*get_random_0_to_1()-1.0);

            delta_energy = evaluate_delta_energy( n_tags, tags, n_points, curve,
                                        tag_rms, nearest_segments,
                                        segment_lengths, point_index,
                                        &n_tag_changes, changed_tag,
                                        changed_tag_rms, new_nearest_segments,
                                        new_segment_lengths );

            new_energy = energy + delta_energy;

#ifdef DEBUG
{
    Real  tmp;
    int   tmp_seg[1000];
    Real  tag_r[1000], seg_len[1000];
    tmp = evaluate_energy( n_tags, tags, n_points, curve, tag_r,
                           tmp_seg, seg_len );
    if( !numerically_close( tmp, new_energy, 0.01 ) )
    {
        print( "error -------------- %g != %g\n", tmp, new_energy );
            delta_energy = evaluate_delta_energy( n_tags, tags, n_points, curve,
                                        tag_rms, nearest_segments,
                                        segment_lengths, point_index,
                                        &n_tag_changes, changed_tag,
                                        changed_tag_rms, new_nearest_segments,
                                        new_segment_lengths );
            tmp = evaluate_energy( n_tags, tags, n_points, curve, tag_r,
                                   tmp_seg, seg_len );
    }
}
#endif

            if( try == 0 )
            {
                min_delta_energy = delta_energy;
                max_delta_energy = delta_energy;
            }
            else if( delta_energy < min_delta_energy )
                min_delta_energy = delta_energy;
            else if( delta_energy > max_delta_energy )
                max_delta_energy = delta_energy;

            if( should_step( delta_energy, temperature ) )
            {
                if( point_index > 0 )
                    segment_lengths[point_index-1] = new_segment_lengths[0];
                if( point_index < n_points-1 )
                    segment_lengths[point_index] = new_segment_lengths[1];

                for_less( change, 0, n_tag_changes )
                {
                    tag_rms[changed_tag[change]] = changed_tag_rms[change];
                    nearest_segments[changed_tag[change]] =
                                              new_nearest_segments[change];
                }

                energy = new_energy;
                ++n_successes;
                if( delta_energy < 0.0 )
                    ++n_pos_successes;
                if( n_successes == max_successes )
                    break;
            }
            else
                curve[point_index] = save_point;
        }

        print( "%3d: %g, %d/%d succ, %d tries, en %g de [%g,%g]\n",
               temperature_step+1, temperature, n_pos_successes, n_successes,
               try+1, energy, min_delta_energy, max_delta_energy );

        temperature *= TEMPERATURE_FACTOR;
        if( n_successes == 0 )
        {
            ++n_no_moves;
            if( n_no_moves == STOP_CRITERIA )
                break;
        }
        else
            n_no_moves = 0;
    }

    energy = evaluate_energy( n_tags, tags, n_points, curve, tag_rms,
                              nearest_segments, segment_lengths );

    rms_error = 0.0;
    for_less( i, 0, n_tags )
        rms_error += tag_rms[i];

    print( "Final energy: %g    Final rms: %g\n", energy, rms_error );

    FREE( tag_rms );
    FREE( changed_tag_rms );
    FREE( segment_lengths );
    FREE( changed_tag );
    FREE( nearest_segments );
    FREE( new_nearest_segments );

    return( rms_error );
}

private  Boolean  should_step(
    Real   delta_energy,
    Real   temperature )
{
    return( delta_energy < 0.0 ||
            get_random_0_to_1() < exp( -delta_energy / temperature ) );
}

private  Real  rms_error(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[],
    Real    tag_rms[],
    int     nearest_segment[] )
{
    int     i;
    Real    sum_rms;

    sum_rms = 0.0;

    for_less( i, 0, n_tags )
    {
        tag_rms[i] = rms_error_of_point( &tags[i], n_points, curve,
                                         &nearest_segment[i] );
        sum_rms += tag_rms[i];
    }

    return( sum_rms );
}

private  Real  curve_length(
    int    n_points,
    Point  points[],
    Real   segment_lengths[] )
{
    int   i;
    Real  len;

    len = 0.0;

    for_less( i, 0, n_points-1 )
    {
        segment_lengths[i] = distance_between_points( &points[i], &points[i+1]);
        len += segment_lengths[i];
    }

    return( len );
}

private  Real  evaluate_energy(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[],
    Real    tag_rms[],
    int     nearest_segments[],
    Real    segment_lengths[] )
{
    Real   energy;

    energy = rms_error( n_tags, tags, n_points, curve, tag_rms,
                        nearest_segments ) / (Real) n_tags +
             CURVE_LENGTH_FACTOR *
             curve_length( n_points, curve, segment_lengths );

    return( energy );
}

private  Real  evaluate_delta_curve_length(
    int     n_points,
    Point   curve[],
    Real    segment_lengths[],
    int     index_changed,
    Real    new_segment_lengths[2] )
{
    Real   delta_len;

    delta_len = 0.0;

    if( index_changed > 0 )
    {
        new_segment_lengths[0] = distance_between_points(
                              &curve[index_changed-1], &curve[index_changed] );
        delta_len += new_segment_lengths[0] - segment_lengths[index_changed-1];
    }

    if( index_changed < n_points-1 )
    {
        new_segment_lengths[1] = distance_between_points(
                              &curve[index_changed], &curve[index_changed+1] );
        delta_len += new_segment_lengths[1] - segment_lengths[index_changed];
    }

    return( delta_len );
}

private  Real  evaluate_delta_avg_rms(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[],
    Real    tag_rms[],
    int     nearest_segments[],
    int     index_changed,
    int     *n_tag_changes,
    int     changed_tag[],
    Real    changed_tag_rms[],
    int     changed_tag_nearest[] )
{
    int     i, c, n_subpoints, start_point, new_nearest_segment;
    Point   min_box, max_box;
    Boolean possible_change, tag_rms_changed;
    Real    delta_rms, current_rms, new_rms;

    min_box = curve[index_changed];
    max_box = curve[index_changed];

    if( index_changed > 0 )
    {
        apply_point_to_min_and_max( &curve[index_changed-1],
                                    &min_box, &max_box );
        start_point = index_changed-1;
        n_subpoints = 2;
    }
    else
    {
        start_point = index_changed;
        n_subpoints = 1;
    }

    if( index_changed < n_points - 1 )
    {
        apply_point_to_min_and_max( &curve[index_changed+1],
                                    &min_box, &max_box );
        ++n_subpoints;
    }

    *n_tag_changes = 0;
    delta_rms = 0.0;
    for_less( i, 0, n_tags )
    {
        current_rms = tag_rms[i];
        tag_rms_changed = FALSE;

        if( nearest_segments[i] == start_point ||
            (n_subpoints == 3 && nearest_segments[i] == start_point+1) )
        {
            new_rms = rms_error_of_point( &tags[i], n_points, curve,
                                          &new_nearest_segment );
            if( new_rms != current_rms )
                tag_rms_changed = TRUE;
        }
        else
        {
            possible_change = TRUE;

/*
            for_less( c, 0, N_DIMENSIONS )
            {
                if( Point_coord(tags[i],c) + current_rms <
                    Point_coord(min_box,c)&&
                    Point_coord(tags[i],c) - current_rms >
                    Point_coord(max_box,c) )
                {
                    possible_change = FALSE;
                    break;
                }
            }
*/

            if( possible_change )
            {
                new_rms = rms_error_of_point( &tags[i], n_subpoints,
                                              &curve[start_point],
                                              &new_nearest_segment );
                new_nearest_segment += start_point;

                if( new_rms < current_rms )
                    tag_rms_changed = TRUE;
            }
        }

        if( tag_rms_changed )
        {
            delta_rms += new_rms - current_rms;
            changed_tag[*n_tag_changes] = i;
            changed_tag_rms[*n_tag_changes] = new_rms;
            changed_tag_nearest[*n_tag_changes] = new_nearest_segment;
            ++(*n_tag_changes);
        }
    }

    return( delta_rms );
}

private  Real  evaluate_delta_energy(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[],
    Real    tag_rms[],
    int     nearest_segments[],
    Real    segment_lengths[],
    int     index_changed,
    int     *n_tag_changes,
    int     changed_tag[],
    Real    changed_tag_rms[],
    int     changed_segments[],
    Real    new_segment_lengths[2] )
{
    Real   delta;

    delta = evaluate_delta_avg_rms( n_tags, tags, n_points, curve, tag_rms,
                                    nearest_segments,
                                    index_changed, n_tag_changes, changed_tag,
                                    changed_tag_rms, changed_segments ) /
                                    (Real) n_tags +
            CURVE_LENGTH_FACTOR * evaluate_delta_curve_length(
                                         n_points, curve,
                                         segment_lengths, index_changed,
                                         new_segment_lengths );

    return( delta );
}

private  Real   rms_error_of_point(
    Point   *point,
    int     n_points,
    Point   curve[],
    int     *nearest_segment )
{
    int   i;
    Real  rms, min_rms;

    min_rms = 0.0;

    for_less( i, 0, n_points-1 )
    {
        rms = rms_to_segment( point, &curve[i], &curve[i+1] );

        if( i == 0 || rms < min_rms )
        {
            min_rms = rms;
            *nearest_segment = i;
        }
    }

    return( min_rms );
}

private  Real  rms_to_segment(
    Point    *point,
    Point    *p1,
    Point    *p2 )
{
    Vector   v, v12, offset;
    Real     mag_v12, rms, ratio;

    SUB_POINTS( v, *point, *p1 );
    SUB_POINTS( v12, *p2, *p1 );

    mag_v12 = DOT_VECTORS( v12, v12 );
    if( mag_v12 == 0.0 )
        mag_v12 = 1.0;

    ratio = DOT_VECTORS( v, v12 ) / mag_v12;

    if( ratio <= 0.0 )
        rms = DOT_VECTORS( v, v );
    else if( ratio >= 1.0 )
    {
        SUB_POINTS( v, *point, *p2 );
        rms = DOT_VECTORS( v, v );
    }
    else
    {
        SCALE_VECTOR( offset, v12, ratio );
        SUB_VECTORS( offset, v, offset );
        rms = DOT_VECTORS( offset, offset );
    }

    return( rms );
}
