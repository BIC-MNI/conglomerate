#include  <def_mni.h>

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_filename  output_filename\n", executable_name );
}

#define  X_SIZE   100
#define  Y_SIZE   100
#define  Z_SIZE   20

#define  X_MIN    -30.0
#define  X_MAX    -10.0
#define  Y_MIN    -20.0
#define  Y_MAX     0.0
#define  Z_MIN     69.0
#define  Z_MAX     71.0

private  void  create_gradient_volume(
    VIO_Volume          volume,
    VIO_BOOL         value_range_present,
    int             min_value,
    int             max_value,
    VIO_Real            gradient_threshold,
    VIO_Real            deriv2_threshold,
    Transform       *voxel_to_world_transform,
    VIO_Volume          *gradient_volume );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_BOOL              value_range_present;
    VIO_Status               status;
    int                  min_value, max_value, sizes[3];
    VIO_Real                 gradient_threshold, deriv2_threshold;
    char                 *input_filename;
    char                 *output_filename;
    VIO_Volume               volume, gradient_volume;
    Minc_file            minc_file;
    Transform            voxel_to_world_transform, convert;
    static VIO_STR        dim_names[] = { MIxspace, MIyspace, MIzspace };

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

    /* read the input volume */

    if( input_volume( input_filename, dim_names, &volume ) != VIO_OK )
        return( 1 );

#ifndef dfsa
    make_translation_transform( X_MIN, Y_MIN, Z_MIN, &voxel_to_world_transform);
    Transform_elem(voxel_to_world_transform,X,X) =
                                         (X_MAX - X_MIN)/(VIO_Real)(X_SIZE-1);
    Transform_elem(voxel_to_world_transform,Y,Y) =
                                         (Y_MAX - Y_MIN)/(VIO_Real)(Y_SIZE-1);
    Transform_elem(voxel_to_world_transform,Z,Z) =
                                         (Z_MAX - Z_MIN)/(VIO_Real)(Z_SIZE-1);
#else
    get_volume_sizes( volume, sizes );
    make_scale_transform( (VIO_Real) (sizes[X]-1) / (VIO_Real) (X_SIZE-1),
                          (VIO_Real) (sizes[Y]-1) / (VIO_Real) (Y_SIZE-1),
                          (VIO_Real) (sizes[Z]-1) / (VIO_Real) (Z_SIZE-1), &convert );

    concat_transforms( &voxel_to_world_transform, &convert,
                       &volume->voxel_to_world_transform );
#endif

    /* --- create the gradient volume */

    create_gradient_volume( volume, value_range_present, min_value, max_value,
                            gradient_threshold, deriv2_threshold,
                            &voxel_to_world_transform, &gradient_volume );

    minc_file = initialize_minc_output( output_filename, 3, dim_names,
                                   gradient_volume->sizes,
                                   gradient_volume->nc_data_type,
                                   gradient_volume->signed_flag,
                                   gradient_volume->min_value,
                                   gradient_volume->max_value,
                                   &voxel_to_world_transform );

    /* --- output the volume */

    status = output_minc_volume( minc_file, gradient_volume );

    if( status == VIO_OK )
        status = close_minc_output( minc_file );

    if( status != VIO_OK )
        print( "Unsuccessful.\n" );

    return( status != VIO_OK );
}

private  void  create_gradient_volume(
    VIO_Volume          volume,
    VIO_BOOL         value_range_present,
    int             min_value,
    int             max_value,
    VIO_Real            gradient_threshold,
    VIO_Real            deriv2_threshold,
    Transform       *voxel_to_world_transform,
    VIO_Volume          *gradient_volume )
{
    VIO_BOOL          within_range, prev_within_range;
    int              gradient_sizes[N_DIMENSIONS];
    int              volume_sizes[N_DIMENSIONS];
    int              x, y, z;
    VIO_Real             dx, dy, dz, value;
    VIO_Real             dxx, dxy, dxz, dyy, dyz, dzz;
    VIO_Real             x_world, y_world, z_world, grad, new_grad;
    VIO_Real             grad_mag, prev_grad, dgx, dgy, dgz;
    progress_struct  progress;

    *gradient_volume = create_volume( 3, volume->dimension_names,
                                      volume->nc_data_type,
                                      volume->signed_flag,
                                      0.0, 255.0 );

    (*gradient_volume)->voxel_to_world_transform = *voxel_to_world_transform;

    get_volume_sizes( volume, volume_sizes );

    gradient_sizes[X] = X_SIZE;
    gradient_sizes[Y] = Y_SIZE;
    gradient_sizes[Z] = Z_SIZE;

    set_volume_size( *gradient_volume, NC_UNSPECIFIED, FALSE, gradient_sizes );
    alloc_volume_data( *gradient_volume );

    initialize_progress_report( &progress, FALSE,
                                gradient_sizes[Z] * gradient_sizes[X],
                                "Creating Gradient" );

    grad = 0.0;
    prev_grad = 0.0;
    prev_within_range = FALSE;

    for_less( z, 0, gradient_sizes[Z] )
    {
        for_less( x, 0, gradient_sizes[X] )
        {
            for_less( y, 0, gradient_sizes[Y] )
            {
                convert_voxel_to_world( *gradient_volume,
                                        (VIO_Real) x, (VIO_Real) y, (VIO_Real) z,
                                        &x_world, &y_world, &z_world );
                (void) evaluate_volume_in_world( volume,
                          x_world, y_world, z_world, 2, FALSE,
                          &value,
                          &dx, &dy, &dz,
                          &dxx, &dxy, &dxz, &dyy, &dyz, &dzz );

                if( x == 0 || x == gradient_sizes[X]-1 ||
                    y == 0 || y == gradient_sizes[Y]-1 ||
                    z == 0 || z == gradient_sizes[Z]-1 )
                {
                    within_range = FALSE;
                }
                else if( value_range_present )
                {
                    within_range = (min_value <= value && value <= max_value);
                }
                else
                    within_range = TRUE;

                value = 0.0;
                if( within_range )
                {
                    grad_mag = dx * dx + dy * dy + dz * dz;
                    if( grad_mag != 0.0 )
                        grad_mag = sqrt( grad_mag );

                    grad = grad_mag - gradient_threshold;

                    if( prev_within_range && prev_grad * grad <= 0.0 )
                    {
                        value = 200.0;
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
                            if( new_grad <
                                deriv2_threshold * grad_mag * grad_mag )
                            {
                                value = 0.0;
                            }
                        }
                    }
                }

                SET_VOXEL_3D( *gradient_volume, x, y, z, value );

                prev_within_range = within_range;
                prev_grad = grad;
            }

            update_progress_report( &progress, z * gradient_sizes[X] + x + 1 );
        }
    }

    terminate_progress_report( &progress );
}
