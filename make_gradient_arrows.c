#include  <def_mni.h>

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_filename  output_filename\n", executable_name );
}

#undef   TWO_DIMENSIONS
#define   TWO_DIMENSIONS

#define  X_SIZE   200
#define  Y_SIZE   200

#define  X_MIN    -40.0
#define  X_MAX    -10.0
#define  Y_MIN    -20.0
#define  Y_MAX      0.0
#define  Z_PLANE   55.0

private  void  create_gradient_lines(
    Volume          volume,
    BOOLEAN         value_range_present,
    int             min_value,
    int             max_value,
    Real            scaling,
    Real            deriv2_scaling,
    int             continuity,
    lines_struct    *lines );

int  main(
    int    argc,
    char   *argv[] )
{
    BOOLEAN              value_range_present;
    Status               status;
    int                  min_value, max_value, continuity;
    Real                 scaling, deriv2_scaling;
    char                 *input_filename;
    char                 *output_filename;
    Volume               volume;
    static STRING        dim_names[] = { MIxspace, MIyspace, MIzspace };
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
    (void) get_real_argument( 1.0, &deriv2_scaling );
    (void) get_int_argument( 0, &continuity );

    /* read the input volume */

    if( input_volume( input_filename, dim_names, &volume ) != OK )
        return( 1 );

    object = create_object( LINES );

    create_gradient_lines( volume, value_range_present, min_value, max_value,
                           scaling, deriv2_scaling, continuity,
                           get_lines_ptr(object) );

    status = output_graphics_file( output_filename, BINARY_FORMAT,
                                   1, &object );

    return( status != OK );
}

private  void  make_arrow(
    lines_struct   *lines,
    Point          *centre,
    Vector         *deriv,
    Vector         *deriv2 )
{
    Point   p;

    start_new_line( lines );
    add_point_to_line( lines, centre );
    ADD_POINT_VECTOR( p, *centre, *deriv );
    add_point_to_line( lines, &p );

    start_new_line( lines );
    add_point_to_line( lines, centre );
    ADD_POINT_VECTOR( p, *centre, *deriv2 );
    add_point_to_line( lines, &p );
}

private  void  compute_curvature(
    Real   dx,
    Real   dy,
    Real   dxx,
    Real   dxy,
    Real   dyy,
    Real   *x_tangent,
    Real   *y_tangent,
    Real   *x_normal,
    Real   *y_normal )
{
    Real   tx, ty, mag_tangent;

    *x_tangent = -dy;
    *y_tangent = dx;

    mag_tangent = sqrt( *x_tangent * *x_tangent + *y_tangent * *y_tangent );

    if( mag_tangent == 0.0 )
        mag_tangent = 1.0;

    tx = *x_tangent / mag_tangent * dxx + *y_tangent / mag_tangent * dxy;
    ty = *x_tangent / mag_tangent * dxy + *y_tangent / mag_tangent * dyy;

    *x_normal = -ty;
    *y_normal = tx;
}

private  void  create_gradient_lines(
    Volume          volume,
    BOOLEAN         value_range_present,
    int             min_value,
    int             max_value,
    Real            scaling,
    Real            deriv2_scaling,
    int             continuity,
    lines_struct    *lines )
{
    BOOLEAN          within_range;
    int              x, y;
    Point            centre;
    Vector           deriv, deriv2;
    Real             dx, dy, dz, value, ignore;
    Real             dxx, dxy, dxz, dyy, dyz, dzz;
    Real             grad_mag, dgx, dgy, dgz;
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

#ifdef TWO_DIMENSIONS
            (void) evaluate_slice_in_world( volume,
                      x_world, y_world, z_world, FALSE,
                      &value, &dx, &dy, &dxx, &dxy, &dyy );
            dz = 0.0;
            dxz = 0.0;
            dyz = 0.0;
            dzz = 0.0;
#else
            (void) evaluate_volume_in_world( volume,
                      x_world, y_world, z_world, continuity, FALSE,
                      &value, &dx, &dy, &dz,
                      &dxx, &dxy, &dxz, &dyy, &dyz, &dzz );
#endif

            if( value_range_present )
            {
                within_range = (min_value <= value && value <= max_value);
            }
            else
                within_range = TRUE;

#ifdef TWO_DIMENSIONS
            if( within_range )
            {
                compute_curvature( dx, dy, dxx, dxy, dyy,
                                   &ignore, &ignore, &dgx, &dgy );

                dx *= scaling;
                dy *= scaling;
                dz = 0.0;
                dgx *= deriv2_scaling;
                dgy *= deriv2_scaling;
                dgz = 0.0;

                fill_Point( centre, x_world, y_world, z_world );
                fill_Vector( deriv, dx, dy, dz );
                fill_Vector( deriv2, dgx, dgy, dgz );

                make_arrow( lines, &centre, &deriv, &deriv2 );
            }
#else
            if( within_range )
            {
                grad_mag = dx * dx + dy * dy + dz * dz;
                if( grad_mag != 0.0 )
                    grad_mag = sqrt( grad_mag );

                dgx = dx / grad_mag * dxx +
                      dy / grad_mag * dxy +
                      dz / grad_mag * dxz;
                dgy = dx / grad_mag * dxy +
                      dy / grad_mag * dyy +
                      dz / grad_mag * dyz;
                dgz = dx / grad_mag * dxz +
                      dy / grad_mag * dyz +
                      dz / grad_mag * dzz;

                dgx *= deriv2_scaling;
                dgy *= deriv2_scaling;
                dgz *= deriv2_scaling;
                dx *= scaling;
                dy *= scaling;
                dz *= scaling;

                fill_Point( centre, x_world, y_world, z_world );
                fill_Vector( deriv, dx, dy, dz );
                fill_Vector( deriv2, dgx, dgy, dgz );

                make_arrow( lines, &centre, &deriv, &deriv2 );
            }
#endif
        }

        update_progress_report( &progress, x + 1 );
    }

    terminate_progress_report( &progress );

    lines->colour_flag = PER_ITEM_COLOURS;
    REALLOC( lines->colours, lines->n_items );
    for_less( x, 0, lines->n_items )
        if( x % 2 == 0 )
            lines->colours[x] = WHITE;
        else
            lines->colours[x] = BLUE;
}
