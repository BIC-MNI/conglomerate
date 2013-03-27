#include  <volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_N_POINTS   100

private  void  match_histograms(
    lines_struct   *lines,
    lines_struct   *target,
    int            n_points,
    VIO_Real           scale,
    VIO_Real           oversize,
    lines_struct   *new_hist,
    lines_struct   *offsets );

private  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s input.obj target.obj output.obj [n_points] [scale]\n\
\n\
     Matches the histograms.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int  argc,
    char *argv[] )
{
    VIO_STR          input_filename, target_filename, output_filename;
    int             n_objects, n_points;
    VIO_Real            scale, oversize;
    File_formats    format;
    object_struct   **object_list, **output_objects;
    lines_struct    *lines, *target;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &target_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( DEFAULT_N_POINTS, &n_points );
    (void) get_real_argument( 1.0, &scale );
    (void) get_real_argument( 0.05, &oversize );

    if( input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list ) != VIO_OK ||
        n_objects != 1 || get_object_type(object_list[0]) != LINES )
    {
        print_error( "File %s must contain one lines object.\n",
                     input_filename );
        return( 1 );
    }

    lines = get_lines_ptr( object_list[0] );

    if( input_graphics_file( target_filename, &format, &n_objects,
                                  &object_list ) != VIO_OK ||
        n_objects != 1 || get_object_type(object_list[0]) != LINES )
    {
        print_error( "File %s must contain one lines object.\n",
                     target_filename );
        return( 1 );
    }

    target = get_lines_ptr( object_list[0] );

    ALLOC( output_objects, 2 );
    output_objects[0] = create_object( LINES );
    output_objects[1] = create_object( LINES );

    match_histograms( lines, target, n_points, scale, oversize,
                      get_lines_ptr( output_objects[0] ),
                      get_lines_ptr( output_objects[1] ) );

    (void) output_graphics_file( output_filename, format, 2, output_objects );

    return( 0 );
}

private  void  match_histograms(
    lines_struct   *lines,
    lines_struct   *target,
    int            n_targets,
    VIO_Real           scale,
    VIO_Real           oversize,
    lines_struct   *new_hist,
    lines_struct   *offsets )
{
    int              extra, p, int_pos, **which, best_which;
    int              point_index, current, prev;
    VIO_Real             min_pos, max_pos, pos, *target_values, *values;
    VIO_Real             min_x, max_x, target_scale, target_trans;
    VIO_Real             used_scale, used_trans, *best, *next_best, *tmp;
    VIO_Real             best_so_far, diff, x;
    VIO_Real             integral, a1, b1, a2, b2;
    VIO_Real             value1, value2;
    progress_struct  progress;

    initialize_lines_with_size( new_hist, WHITE, lines->n_points, FALSE );
    initialize_lines( offsets, GREEN );

    min_x = (VIO_Real) Point_x(target->points[0]);
    max_x = (VIO_Real) Point_x(target->points[target->n_points-1]);

    target_scale = (max_x - min_x) / (VIO_Real) (target->n_points-1);
    target_trans = min_x;

    extra = ROUND( (VIO_Real) target->n_points * oversize );

    if( n_targets <= 1 )
        n_targets = target->n_points + 2 * extra;

    if( n_targets < lines->n_points )
        n_targets = lines->n_points;

    min_pos = (VIO_Real) - extra;
    max_pos = (VIO_Real) (target->n_points - 1 + extra);

    used_scale = target_scale * (max_pos - min_pos) / (VIO_Real) (n_targets-1);
    used_trans = target_scale * min_pos + target_trans;

    ALLOC( target_values, n_targets );
    for_less( p, 0, n_targets )
    {
        pos = VIO_INTERPOLATE( (VIO_Real) p / (VIO_Real) (n_targets-1), min_pos, max_pos );
        int_pos = VIO_FLOOR( pos );
        if( int_pos == target->n_points-1 && pos == (VIO_Real) int_pos )
            --int_pos;

        if( int_pos < 0 || int_pos >= target->n_points-1 )
            target_values[p] = 0.0;
        else
        {
            target_values[p] = ((VIO_Real) (int_pos+1) - pos) *
                                    (VIO_Real) Point_y(target->points[int_pos]) +
                               (pos - (VIO_Real) int_pos) *
                                    (VIO_Real) Point_y(target->points[int_pos+1]);
        }
    }

    ALLOC( values, lines->n_points );
    for_less( p, 0, lines->n_points )
        values[p] = scale * (VIO_Real) Point_y(lines->points[p]);

    ALLOC( best, n_targets );
    ALLOC( next_best, n_targets );
    ALLOC2D( which, lines->n_points, n_targets );

    for_less( p, 0, n_targets )
    {
        best[p] = 0.0;
    }

    initialize_progress_report( &progress, FALSE, lines->n_points-1,
                                "Minimizing" );

    for_less( point_index, 1, lines->n_points )
    {
        value1 = values[point_index-1];
        value2 = values[point_index];

        for_less( current, point_index, n_targets )
        {
            for_less( prev, point_index-1, current )
            {
                integral = 0.0;
                for_less( p, prev, current )
                {
                    a1 = VIO_INTERPOLATE( (VIO_Real) (p-prev) / (VIO_Real)(current-prev),
                                      value1, value2 );
                    a2 = VIO_INTERPOLATE( (VIO_Real) (p-prev+1) / (VIO_Real)(current-prev),
                                      value1, value2 );
                    b1 = target_values[p] - a1;
                    b2 = target_values[p+1] - a2;

                    if( b1 != b2 )
                    {
                        integral += (b1 * b1 + b2 * b2 + b1 * b2) / 3.0;
                    }
                }

                diff = best[prev] + integral;

                if( prev == point_index-1 || diff < next_best[current] )
                {
                    next_best[current] = diff;
                    which[point_index][current] = prev;
                }
            }
        }

        tmp = best;
        best = next_best;
        next_best = tmp;

        update_progress_report( &progress, point_index );
    }

    terminate_progress_report( &progress );

    best_so_far = 0.0;
    for_less( p, lines->n_points-1, n_targets )
    {
        if( p == lines->n_points-1 || best[p] < best_so_far )
        {
            best_so_far = best[p];
            best_which = p;
        }
    }

    print( "Value: %g\n", best_so_far );

    for_down( p, lines->n_points-1, 0 )
    {
        x = used_scale * (VIO_Real) best_which + used_trans;
        fill_Point( new_hist->points[p],
                    x,
                    scale * (VIO_Real) Point_y(lines->points[p]),
                    Point_z(lines->points[p]) );

        best_which = which[p][best_which];
    }

    FREE( which );
    FREE( best );
    FREE( next_best );
    FREE( target_values );
    FREE( values );
}
