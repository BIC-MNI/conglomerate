#include  <bicpl.h>
#include  <volume_io/internal_volume_io.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input_lines.obj  output_quadmesh.obj  x_norm y_norm z_norm \n\
                min_dist  max_dist  n_along_line  n_along_norm  [degrees]\n\
\n\
     Extrudes a line in the direction of the normal.\n\n";

    print_error( usage_str, executable );
}

private  void   extrude_lines(
    lines_struct      *lines,
    Vector            *normal,
    Real              min_dist,
    Real              max_dist,
    int               n_along_line,
    int               n_along_norm,
    int               degrees_continuity,
    quadmesh_struct   *quadmesh );

#define  DEFAULT_DEGREES_CONTINUITY   2

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    File_formats         format;
    int                  n_along_line, n_along_norm;
    int                  n_objects, degrees_continuity;
    Real                 xn, yn, zn, min_dist, max_dist;
    Vector               normal;
    lines_struct         *lines;
    object_struct        **objects, *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &xn ) ||
        !get_real_argument( 0.0, &yn ) ||
        !get_real_argument( 0.0, &zn ) ||
        !get_real_argument( 0.0, &min_dist ) ||
        !get_real_argument( 0.0, &max_dist ) ||
        !get_int_argument( 0, &n_along_line ) ||
        !get_int_argument( 0, &n_along_norm ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( DEFAULT_DEGREES_CONTINUITY, &degrees_continuity );

    if( input_graphics_file( input_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects != 1 || get_object_type(objects[0]) != LINES )
    {
        print_error( "Must contain one lines structure.\n" );
        return( 1 );
    }

    lines = get_lines_ptr( objects[0] );

    if( lines->n_items != 1 )
    {
        print_error( "Must contain a single line.\n" );
        return( 1 );
    }

    object = create_object( QUADMESH );

    fill_Vector( normal, xn, yn, zn );

    extrude_lines( lines, &normal, min_dist, max_dist,
                   n_along_line, n_along_norm,
                   degrees_continuity, get_quadmesh_ptr(object) );

    if( output_graphics_file( output_filename, format, 1,
                              &object ) != OK )
        return( 1 );

    delete_object_list( n_objects, objects );
    delete_object( object );

    return( 0 );
}

private  void  evaluate_line(
    lines_struct   *lines,
    Real           parametric_dist,
    int            degrees_continuity,
    Point          *point,
    Vector         *line_normal )
{
    int    p, size, this_index, next_index, i0, i3, dim;
    Real   dist, inc_dist, ratio;
    Real   coefs[4], derivs[2];
    
    size = GET_OBJECT_SIZE( *lines, 0 );

    dist = 0.0;

    for_less( p, 0, size-1 )
    {
        this_index = lines->indices[POINT_INDEX( lines->end_indices, 0, p )];
        next_index = lines->indices[POINT_INDEX( lines->end_indices, 0, p+1 )];

        inc_dist = distance_between_points( &lines->points[this_index],
                                            &lines->points[next_index] );

        if( p == size-1 || parametric_dist < dist + inc_dist )
        {
            ratio = (parametric_dist - dist) / inc_dist;
            if( ratio < 0.0 )
                ratio = 0.0;
            else if( ratio > 1.0 )
                ratio = 1.0;
            break;
        }

        dist += inc_dist;
    }

    if( degrees_continuity == 2 )
    {
        i0 = MAX( 0, this_index-1 );
        i3 = MIN( size-1, this_index+2 );

        for_less( dim, 0, N_DIMENSIONS )
        {
            coefs[0] = Point_coord( lines->points[i0], dim );
            coefs[1] = Point_coord( lines->points[this_index], dim );
            coefs[2] = Point_coord( lines->points[next_index], dim );
            coefs[3] = Point_coord( lines->points[i3], dim );

            evaluate_univariate_interpolating_spline( ratio,
                                                      degrees_continuity + 2,
                                                      coefs, 1, derivs );

            Point_coord( *point, dim ) = derivs[0];
            Vector_coord( *line_normal, dim ) = derivs[1];
        }
    }
    else
    {
        SUB_POINTS( *line_normal, lines->points[next_index],
                                  lines->points[this_index] );

        INTERPOLATE_POINTS( *point, lines->points[this_index],
                                    lines->points[next_index], ratio );
    }

    NORMALIZE_VECTOR( *line_normal, *line_normal );
}

private  void   extrude_lines(
    lines_struct      *lines,
    Vector            *normal,
    Real              min_dist,
    Real              max_dist,
    int               n_along_line,
    int               n_along_norm,
    int               degrees_continuity,
    quadmesh_struct   *quadmesh )
{
    int      l, d;
    Vector   unit_normal, line_normal, surface_normal;
    Real     total_length, dist;
    Point    line_point, point;

    total_length = get_lines_length( lines );

    NORMALIZE_VECTOR( unit_normal, *normal );

    initialize_quadmesh( quadmesh, WHITE, NULL, n_along_line, n_along_norm );

    for_less( l, 0, n_along_line )
    {
        dist = (Real) l / (Real) (n_along_line-1) * total_length;
        evaluate_line( lines, dist, degrees_continuity,
                       &line_point, &line_normal );
        CROSS_VECTORS( surface_normal, line_normal, unit_normal );
        NORMALIZE_VECTOR( surface_normal, surface_normal );

        for_less( d, 0, n_along_norm )
        {
            dist = INTERPOLATE( (Real) d / (Real) (n_along_norm-1),
                                min_dist, max_dist );

            GET_POINT_ON_RAY( point, line_point, unit_normal, dist );

            set_quadmesh_point( quadmesh, l, d, &point, &surface_normal );
        }
    }
}
