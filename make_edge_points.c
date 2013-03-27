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
    VIO_Volume          volume,
    VIO_BOOL         value_range_present,
    int             min_value,
    int             max_value,
    VIO_Real            gradient_threshold,
    VIO_BOOL         deriv2_present,
    VIO_Real            deriv2_threshold,
    int             continuity,
    lines_struct    *lines );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_BOOL              value_range_present;
    VIO_Status               status;
    int                  min_value, max_value, continuity;
    VIO_Real                 gradient_threshold, deriv2_threshold;
    VIO_BOOL              deriv2_present;
    char                 *input_filename;
    char                 *output_filename;
    VIO_Volume               volume;
    static VIO_STR        dim_names[] = { MIxspace, MIyspace, MIzspace };
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
    deriv2_present = get_real_argument( 0.0, &deriv2_threshold );
    (void) get_int_argument( 2, &continuity );

    /* read the input volume */

    if( input_volume( input_filename, dim_names, &volume ) != VIO_OK )
        return( 1 );

    object = create_object( LINES );

    create_gradient_lines( volume, value_range_present, min_value, max_value,
                           gradient_threshold, deriv2_present,
                           deriv2_threshold, continuity,
                           get_lines_ptr(object) );

    status = output_graphics_file( output_filename, BINARY_FORMAT,
                                   1, &object );

    return( status != VIO_OK );
}

private  void  create_gradient_lines(
    VIO_Volume          volume,
    VIO_BOOL         value_range_present,
    int             min_value,
    int             max_value,
    VIO_Real            gradient_threshold,
    VIO_BOOL         deriv2_present,
    VIO_Real            deriv2_threshold,
    int             continuity,
    lines_struct    *lines )
{
    VIO_BOOL          within_range, add_point;
    int              x, y, sizes[N_DIMENSIONS];
    VIO_Real             dx, dy, dz, value, dot_product, prev_dot_product;
    VIO_Real             dxx, dxy, dxz, dyy, dyz, dzz;
    VIO_Real             grad, new_grad;
    VIO_Real             x_world, y_world, z_world;
    VIO_Real             grad_mag, prev_grad, dgx, dgy, dgz, mag_deriv2;
    VIO_Real             *dxx_ptr;
    VIO_Point            point;
    progress_struct  progress;

    initialize_lines( lines, WHITE );

    initialize_progress_report( &progress, FALSE, X_SIZE,
                                "Creating Gradient Lines" );

    if( deriv2_present )
        dxx_ptr = &dxx;
    else
        dxx_ptr = (VIO_Real *) NULL;

    get_volume_sizes( volume, sizes );
    grad = 0.0;
    prev_grad = 0.0;
    prev_dot_product = 0.0;

    for_less( x, 0, X_SIZE )
    {
        x_world = X_MIN + (X_MAX - X_MIN) * (VIO_Real) x / (VIO_Real) (X_SIZE-1);
        for_less( y, 0, Y_SIZE )
        {
            y_world = Y_MIN + (Y_MAX - Y_MIN) * (VIO_Real) y / (VIO_Real) (Y_SIZE-1);
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

            if( grad >= 0.0 && deriv2_present )
            {
                if( grad_mag == 0.0 )
                    grad_mag = 1.0;

                dgx = dx / grad_mag * dxx +
                      dy / grad_mag * dxy +
                      dz / grad_mag * dxz;
                dgy = dx / grad_mag * dxy +
                      dy / grad_mag * dyy +
                      dz / grad_mag * dyz;
                dgz = dx / grad_mag * dxz +
                      dy / grad_mag * dyz +
                      dz / grad_mag * dzz;

                mag_deriv2 = sqrt( dgx * dgx + dgy * dgy + dgz * dgz );
                if( mag_deriv2 == 0.0 )
                    mag_deriv2 = 1.0;

                dot_product = (dgx * dx +
                               dgy * dy +
                               dgz * dz) / mag_deriv2 / grad_mag -
                               deriv2_threshold;
            }

            add_point = FALSE;

            if( prev_grad * grad <= 0.0 )
            {
                if( deriv2_present )
                {
                    if( grad >= 0.0 && dot_product >= 0.0 ||
                        prev_grad >= 0.0 && prev_dot_product >= 0.0 )
                        add_point = TRUE;
                }
                else
                    add_point = TRUE;
            }
            else if( grad >= 0.0 )
            {
                if( deriv2_present &&
                    grad >= 0.0 && dot_product >= 0.0 ||
                    prev_grad >= 0.0 && prev_dot_product >= 0.0 )
                    add_point = TRUE;
            }

            add_point = FALSE;
            if( grad >= 0.0 && (!deriv2_present || dot_product >= 0.0) )
                add_point = TRUE;

            if( add_point )
            {
                start_new_line( lines );
                fill_Point( point, x_world, y_world, z_world );
                add_point_to_line( lines, &point );
            }

            prev_grad = grad;
            prev_dot_product = dot_product;
        }

        update_progress_report( &progress, x + 1 );
    }

    terminate_progress_report( &progress );
}
