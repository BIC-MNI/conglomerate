#include  <def_mni.h>

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_filename  output_filename\n", executable_name );
}

#define  X_SIZE   200
#define  Y_SIZE   200

#define  X_MIN    -30.0
#define  X_MAX      0.0
#define  Y_MIN    -20.0
#define  Y_MAX     10.0
#define  Z_PLANE   70.0

private  void  create_gradient_lines(
    Volume          volume,
    Boolean         value_range_present,
    int             min_value,
    int             max_value,
    Real            scaling,
    int             continuity,
    lines_struct    *lines );

int  main(
    int    argc,
    char   *argv[] )
{
    Boolean              value_range_present;
    Status               status;
    int                  min_value, max_value, continuity;
    Real                 scaling;
    char                 *input_filename;
    char                 *output_filename;
    Volume               volume;
    static String        dim_names[] = { MIxspace, MIyspace, MIzspace };
    object_struct        *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( get_int_argument( 0, &min_value ) && get_int_argument( 0, &max_value ) )
        value_range_present = TRUE;
    else
        value_range_present = FALSE;

    (void) get_real_argument( 1.0, &scaling );
    (void) get_int_argument( 0, &continuity );

    /* read the input volume */

    if( input_volume( input_filename, dim_names, &volume ) != OK )
        return( 1 );

    object = create_object( LINES );

    create_gradient_lines( volume, value_range_present, min_value, max_value,
                           scaling, continuity, get_lines_ptr(object) );

    status = output_graphics_file( output_filename, BINARY_FORMAT,
                                   1, &object );

    return( status != OK );
}

private  void  make_arrow(
    lines_struct   *lines,
    Point          *centre,
    Point          *offset )
{
    Point   p;
    Vector  diff;

    add_point_to_line( lines, centre );
    add_point_to_line( lines, offset );

    SUB_POINTS( diff, *offset, *centre );

    fill_Point( p, Point_x(*offset) - 0.15 * Vector_y(diff),
                   Point_y(*offset) + 0.15 * Vector_x(diff),
                   Point_z(*offset) );

    add_point_to_line( lines, &p );
}

private  void  create_gradient_lines(
    Volume          volume,
    Boolean         value_range_present,
    int             min_value,
    int             max_value,
    Real            scaling,
    int             continuity,
    lines_struct    *lines )
{
    Boolean          within_range;
    int              x, y;
    Point            centre, offset;
    Real             dx, dy, dz, value;
    Real             x_world, y_world, z_world;
    progress_struct  progress;

    initialize_lines( lines, WHITE );

    initialize_progress_report( &progress, FALSE, X_SIZE,
                                "Creating Gradient Lines" );

    for_less( x, 0, X_SIZE )
    {
        x_world = X_MIN + (X_MAX - X_MIN) * (Real) x / (Real) (X_SIZE-1);
        for_less( y, 0, Y_SIZE )
        {
            y_world = Y_MIN + (Y_MAX - Y_MIN) * (Real) y / (Real) (Y_SIZE-1);
            z_world = Z_PLANE;

            (void) evaluate_volume_in_world( volume,
                      x_world, y_world, z_world, continuity, FALSE,
                      &value, &dx, &dy, &dz );

            if( value_range_present )
            {
                within_range = (min_value <= value && value <= max_value);
            }
            else
                within_range = TRUE;

            if( within_range )
            {
                dx *= scaling;
                dy *= scaling;
                dz *= scaling;

                fill_Point( centre, x_world, y_world, z_world );
                fill_Point( offset, x_world + dx, y_world + dy, z_world + dz );

                start_new_line( lines );
                make_arrow( lines, &centre, &offset );
            }
        }

        update_progress_report( &progress, x + 1 );
    }

    terminate_progress_report( &progress );
}
