#include  <def_mni.h>

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_filename  output_filename\n", executable_name );
}

#define  X_SIZE   400
#define  Y_SIZE   400

#define  X_MIN    -40.0
#define  X_MAX    -10.0
#define  Y_MIN    -20.0
#define  Y_MAX      0.0
#define  Z_PLANE   55.0

private  void  create_gradient_lines(
    Volume          volume,
    Boolean         value_range_present,
    int             min_value,
    int             max_value,
    Real            gradient_threshold,
    Real            deriv2_threshold,
    int             continuity,
    lines_struct    *lines );

int  main(
    int    argc,
    char   *argv[] )
{
    Boolean              value_range_present;
    Status               status;
    int                  min_value, max_value, continuity;
    Real                 gradient_threshold, deriv2_threshold;
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

    (void) get_real_argument( 1.0, &gradient_threshold );
    (void) get_real_argument( 0.0, &deriv2_threshold );
    (void) get_int_argument( 2, &continuity );

    /* read the input volume */

    if( input_volume( input_filename, dim_names, &volume ) != OK )
        return( 1 );

    object = create_object( LINES );

    create_gradient_lines( volume, value_range_present, min_value, max_value,
                           gradient_threshold, deriv2_threshold, continuity,
                           get_lines_ptr(object) );

    status = output_graphics_file( output_filename, BINARY_FORMAT,
                                   1, &object );

    return( status != OK );
}

private  void  create_gradient_lines(
    Volume          volume,
    Boolean         value_range_present,
    int             min_value,
    int             max_value,
    Real            gradient_threshold,
    Real            deriv2_threshold,
    int             continuity,
    lines_struct    *lines )
{
    Boolean          within_range, add_point;
    int              x, y, sizes[N_DIMENSIONS];
    Real             dx, dy, dz, value;
    Real             dxx, dxy, dxz, dyy, dyz, dzz;
    Real             grad, new_grad;
    Real             x_world, y_world, z_world;
    Real             grad_mag, prev_grad, dgx, dgy, dgz;
    Real             *dxx_ptr;
    Point            point;
    progress_struct  progress;

    initialize_lines( lines, WHITE );

    initialize_progress_report( &progress, FALSE, X_SIZE,
                                "Creating Gradient Lines" );

    if( deriv2_threshold > 0.0 )
        dxx_ptr = &dxx;
    else
        dxx_ptr = (Real *) NULL;

    get_volume_sizes( volume, sizes );
    grad = 0.0;
    prev_grad = 0.0;

    for_less( x, 0, X_SIZE )
    {
        x_world = X_MIN + (X_MAX - X_MIN) * (Real) x / (Real) (X_SIZE-1);
        for_less( y, 0, Y_SIZE )
        {
            y_world = Y_MIN + (Y_MAX - Y_MIN) * (Real) y / (Real) (Y_SIZE-1);
            z_world = Z_PLANE;

            (void) evaluate_volume_in_world( volume,
                      x_world, y_world, z_world, continuity, FALSE,
                      &value, &dx, &dy, &dz,
                      dxx_ptr, &dxy, &dxz, &dyy, &dyz, &dzz );

            if( x == 0 || x == X_SIZE-1 ||
                y == 0 || y == Y_SIZE-1 )
            {
                within_range = FALSE;
            }
            else if( value_range_present )
            {
                within_range = (min_value <= value && value <= max_value);
            }
            else
                within_range = TRUE;

            if( within_range )
            {
                grad_mag = dx * dx + dy * dy + dz * dz;
                if( grad_mag != 0.0 )
                    grad_mag = sqrt( grad_mag );
            }
            else
            {
                grad_mag = 0.0;
            }

            grad = grad_mag - gradient_threshold;

            add_point = FALSE;

            if( prev_grad * grad <= 0.0 )
            {
                if( deriv2_threshold > 0.0 )
                {
                    dgx = dx / grad_mag * dxx +
                          dy / grad_mag * dxy +
                          dz / grad_mag * dxz;
                    dgy = dx / grad_mag * dxy +
                          dy / grad_mag * dyy +
                          dz / grad_mag * dyz;
                    dgz = dx / grad_mag * dxz +
                          dy / grad_mag * dyz +
                          dz / grad_mag * dzz;
                    new_grad = (dx + dgx) * (dx + dgx) +
                               (dy + dgy) * (dy + dgy) +
                               (dz + dgz) * (dz + dgz);
                    if( new_grad >= deriv2_threshold * grad_mag * grad_mag )
                        add_point = TRUE;
                }
                else
                    add_point = TRUE;
            }

            if( add_point )
            {
                start_new_line( lines );
                fill_Point( point, x_world, y_world, z_world );
                add_point_to_line( lines, &point );
            }

            prev_grad = grad;
        }

        update_progress_report( &progress, x + 1 );
    }

    terminate_progress_report( &progress );
}
