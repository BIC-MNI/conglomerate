#include  <def_mni.h>

private  int  create_points(
    int            n_objects,
    object_struct  *object_list[],
    Point          *points[] );
private  void   fit_line_to_points(
    int            n_points,
    Point          points[],
    int            max_points,
    lines_struct   *lines );
private  int  create_curve_approximating_points(
    int            n_tags,
    Point          tags[],
    int            max_points,
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
    Point   curve[] );
private  Real   distance_point_to_curve(
    Point   *point,
    int     n_points,
    Point   curve[] );
private  Real  distance_point_to_segment(
    Point    *point,
    Point    *p1,
    Point    *p2 );
private  Real  curve_length(
    int    n_points,
    Point  points[] );

#define  DEFAULT_MAX_POINTS  10

int   main(
    int   argc,
    char  *argv[] )
{
    Status         status;
    char           *input_filename, *output_filename;
    int            n_objects, n_points, max_points;
    Point          *points;
    object_struct  **object_list, *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        (void) fprintf( stderr, "Require input and output filename.\n" );
        return( 1 );
    }

    (void) get_int_argument( DEFAULT_MAX_POINTS, &max_points );

    status = input_landmark_file( (Volume) NULL, input_filename,
                                  WHITE, 1.0, BOX_MARKER,
                                  &n_objects, &object_list );

    if( status != OK )
        return( 1 );

    n_points = create_points( n_objects, object_list, &points );

    delete_object_list( n_objects, object_list );

    object = create_object( LINES );

    fit_line_to_points( n_points, points, max_points, get_lines_ptr(object) );

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
    lines_struct   *lines )
{
    int    i, n_curve_points;
    Point  *curve_points;

    n_curve_points = create_curve_approximating_points( n_points, points,
                                                   max_points, &curve_points );

    initialize_lines( lines, WHITE );

    for_less( i, 0, n_curve_points )
        add_point_to_line( lines, &curve_points[i] );
}

#define  MAX_ERROR     0.0

private  int  create_curve_approximating_points(
    int            n_tags,
    Point          tags[],
    int            max_points,
    Point          *curve_points[] )
{
    int      i, c, n_points;
    Boolean  first_time;
    Point    *curve, *tmp;
    Real     error;

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

    error = 2.0 * MAX_ERROR;
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

        error = fit_curve( n_tags, tags, n_points, curve );
    }
    while( n_points < max_points && error >= MAX_ERROR );

    *curve_points = curve;

    return( n_points );
}

#define  INITIAL_TEMPERATURE       0.01
#define  TEMPERATURE_FACTOR        0.9
#define  NUM_TEMPERATURE_STEPS     50
#define  MAX_TRIES_FACTOR          100
#define  MAX_SUCCESSES_FACTOR      10
#define  TAG_DIST_FACTOR           0.1

#define  STOP_CRITERIA             3

#define  RANDOM_MOVEMENT_DISTANCE  0.5

private  Real  fit_curve(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[] )
{
    int    n_no_moves;
    Real   energy, new_energy, temperature, random_distance;
    Real   delta_energy, min_delta_energy, max_delta_energy;
    Point  save_point;
    int    temperature_step, try, point_index, n_successes, max_tries;
    int    max_successes, n_pos_successes;

    max_tries = MAX_TRIES_FACTOR * n_points;
    max_successes = MAX_SUCCESSES_FACTOR * n_points;

    temperature = INITIAL_TEMPERATURE;

    min_delta_energy = 0.0;
    max_delta_energy = 0.0;

    energy = evaluate_energy( n_tags, tags, n_points, curve );

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

            new_energy = evaluate_energy( n_tags, tags, n_points, curve );

            delta_energy = new_energy - energy;

/*
            print( "energy: %g - %g = %g\n", new_energy, energy, delta_energy );
*/
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

    energy = evaluate_energy( n_tags, tags, n_points, curve );
    print( "Final energy: %g\n", energy );

    return( energy );
}

private  Boolean  should_step(
    Real   delta_energy,
    Real   temperature )
{
    return( delta_energy < 0.0 ||
            get_random_0_to_1() < exp( -delta_energy / temperature ) );
}

private  Real  avg_distance_to_curve(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[] )
{
    int     i;
    Real    sum_distance;

    sum_distance = 0.0;

    for_less( i, 0, n_tags )
    {
        sum_distance += distance_point_to_curve( &tags[i], n_points, curve );
    }

    return( sum_distance / (Real) n_tags );
}

private  Real  avg_distance_to_tag(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[] )
{
    int     i, t;
    Real    sum_distance, dist, min_distance;

    sum_distance = 0.0;

    for_less( i, 0, n_points )
    {
        min_distance = 0.0;
        for_less( t, 0, n_tags )
        {
            dist = distance_between_points( &curve[i], &tags[t] );
            if( t == 0 || dist < min_distance )
                min_distance = dist;
        }

        sum_distance += min_distance;
    }

    return( sum_distance / (Real) n_points );
}

private  Real  evaluate_energy(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[] )
{
    Real   energy;

    energy = avg_distance_to_curve( n_tags, tags, n_points, curve ) +
             TAG_DIST_FACTOR *
             avg_distance_to_tag( n_tags, tags, n_points, curve );

    return( energy );
}

private  Real  evaluate_delta_energy(
    int     n_tags,
    Point   tags[],
    int     n_points,
    Point   curve[] )
{
}

private  Real   distance_point_to_curve(
    Point   *point,
    int     n_points,
    Point   curve[] )
{
    int   i;
    Real  distance, min_distance;

    min_distance = 0.0;

    for_less( i, 0, n_points-1 )
    {
        distance = distance_point_to_segment( point, &curve[i], &curve[i+1] );

        if( i == 0 || distance < min_distance )
            min_distance = distance;
    }

    return( min_distance );
}

private  Real  distance_point_to_segment(
    Point    *point,
    Point    *p1,
    Point    *p2 )
{
    Vector   v, v12, offset;
    Real     mag_v12, dist, ratio;

    SUB_POINTS( v, *point, *p1 );
    SUB_POINTS( v12, *p2, *p1 );

    mag_v12 = DOT_VECTORS( v12, v12 );
    if( mag_v12 == 0.0 )
        mag_v12 = 1.0;

    ratio = DOT_VECTORS( v, v12 ) / mag_v12;

    if( ratio <= 0.0 )
        dist = distance_between_points( point, p1 );
    else if( ratio >= 1.0 )
        dist = distance_between_points( point, p2 );
    else
    {
        SCALE_VECTOR( offset, v12, ratio );
        SUB_VECTORS( offset, v, offset );
        dist = MAGNITUDE( offset );
    }

    return( dist );
}

private  Real  curve_length(
    int    n_points,
    Point  points[] )
{
    int   i;
    Real  len;

    len = 0.0;

    for_less( i, 0, n_points-1 )
        len += distance_between_points( &points[i], &points[i+1] );

    return( len );
}
