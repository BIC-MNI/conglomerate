#include  <def_mni.h>

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_filename  output_filename\n", executable_name );
}

#define  X_SIZE   100
#define  Y_SIZE   100
#define  Z_SIZE   9

#define  X_MIN    -40.0
#define  X_MAX    -10.0
#define  Y_MIN    -20.0
#define  Y_MAX     0.0
#define  Z_MIN     50.0
#define  Z_MAX     60.0

private  void  create_gradient_volume(
    Volume          volume,
    BOOLEAN         value_range_present,
    int             min_value,
    int             max_value,
    Real            scaling,
    int             continuity,
    Transform       *voxel_to_world_transform,
    Volume          *gradient_volume );

int  main(
    int    argc,
    char   *argv[] )
{
    BOOLEAN              value_range_present;
    Status               status;
    int                  min_value, max_value, continuity, sizes[3];
    Real                 scaling;
    char                 *input_filename;
    char                 *output_filename;
    Volume               volume, gradient_volume;
    Minc_file            minc_file;
    Transform            voxel_to_world_transform, convert;
    static STRING        dim_names[] = { MIxspace, MIyspace, MIzspace };

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

#ifndef dfsa
    make_translation_transform( X_MIN, Y_MIN, Z_MIN, &voxel_to_world_transform);
    Transform_elem(voxel_to_world_transform,X,X) =
                                         (X_MAX - X_MIN)/(Real)(X_SIZE-1);
    Transform_elem(voxel_to_world_transform,Y,Y) =
                                         (Y_MAX - Y_MIN)/(Real)(Y_SIZE-1);
    Transform_elem(voxel_to_world_transform,Z,Z) =
                                         (Z_MAX - Z_MIN)/(Real)(Z_SIZE-1);
#else
    get_volume_sizes( volume, sizes );
    make_scale_transform( (Real) (sizes[X]-1) / (Real) (X_SIZE-1),
                          (Real) (sizes[Y]-1) / (Real) (Y_SIZE-1),
                          (Real) (sizes[Z]-1) / (Real) (Z_SIZE-1), &convert );

    concat_transforms( &voxel_to_world_transform, &convert,
                       &volume->voxel_to_world_transform );
#endif

    /* --- create the gradient volume */

    create_gradient_volume( volume, value_range_present, min_value, max_value,
                            scaling, continuity, &voxel_to_world_transform,
                            &gradient_volume );

    minc_file = initialize_minc_output( output_filename, 3, dim_names,
                                   gradient_volume->sizes,
                                   gradient_volume->nc_data_type,
                                   gradient_volume->signed_flag,
                                   gradient_volume->min_value,
                                   gradient_volume->max_value,
                                   &voxel_to_world_transform );

    /* --- output the volume */

    status = output_minc_volume( minc_file, gradient_volume );

    if( status == OK )
        status = close_minc_output( minc_file );

    if( status != OK )
        print( "Unsuccessful.\n" );

    return( status != OK );
}

private  void  create_gradient_volume(
    Volume          volume,
    BOOLEAN         value_range_present,
    int             min_value,
    int             max_value,
    Real            scaling,
    int             continuity,
    Transform       *voxel_to_world_transform,
    Volume          *gradient_volume )
{
    BOOLEAN          within_range;
    int              gradient_sizes[N_DIMENSIONS];
    int              volume_sizes[N_DIMENSIONS];
    int              x, y, z;
    Real             dx, dy, dz, value;
    Real             x_world, y_world, z_world, grad;
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
                                gradient_sizes[X] * gradient_sizes[Y],
                                "Creating Gradient" );

    for_less( x, 0, gradient_sizes[X] )
    {
        for_less( y, 0, gradient_sizes[Y] )
        {
            for_less( z, 0, gradient_sizes[Z] )
            {
                convert_voxel_to_world( *gradient_volume,
                                        (Real) x, (Real) y, (Real) z,
                                        &x_world, &y_world, &z_world );
                (void) evaluate_volume_in_world( volume,
                          x_world, y_world, z_world, continuity, FALSE,
                          &value, &dx, &dy, &dz,
                          (Real *) NULL, (Real *) NULL, (Real *) NULL,
                          (Real *) NULL, (Real *) NULL, (Real *) NULL );

                if( value_range_present )
                {
                    within_range = (min_value <= value && value <= max_value);
                }
                else
                    within_range = TRUE;

                if( within_range )
                {
                    if( scaling <= 0.0 )
                    {
                        grad = value;
                    }
                    else
                    {
                        grad = dx * dx + dy * dy + dz * dz;
                        if( grad != 0.0 )  grad = sqrt( grad );

                        grad *= scaling;

                        if( grad > 255.0 )
                            grad = 255.0;
                    }
                }
                else
                    grad = 0.0;

                SET_VOXEL_3D( *gradient_volume, x, y, z, ROUND( grad ) );
            }

            update_progress_report( &progress, x * gradient_sizes[Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );
}
