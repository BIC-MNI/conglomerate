#include  <math.h>
#include  <bicpl.h>
#include  <volume_io.h>

static  void  usage(
    VIO_STR   executable_name )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input.mnc  output.mnc [1|2] [1|3]\n\
\n\
     Creates a magnitude of gradient or second derivative of the volume,\n\
     depending on whether 1 or 2 is specified.  The choice of 1 or 3 selects\n\
     linear or cubic interpolation.  If the second derivative is chosen,\n\
     cubic interpolation will be used.\n\n";

     print_error( usage_str, executable_name );

}

static  VIO_Volume  create_gradient_volume(
    VIO_Volume          volume,
    int             continuity,
    int             deriv_number );

int  main(
    int    argc,
    char   *argv[] )
{
    int                  deriv_number, order;
    VIO_STR               input_filename;
    VIO_STR               output_filename;
    VIO_STR               history;
    VIO_Volume               volume, gradient_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1, &deriv_number );
    (void) get_int_argument( 1, &order );

    if( deriv_number != 1 && deriv_number != 2 || order != 1 && order != 3 )
    {
        usage( argv[0] );
        return( 1 );
    }

    /* read the input volume */

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    /* --- create the gradient volume */

    gradient_volume = create_gradient_volume( volume, order-1, deriv_number );

    history = create_string( "make_gradient_volume\n" );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                           0.0, 0.0, gradient_volume, input_filename,
                           history, (minc_output_options *) NULL );


    return( 0 );
}

static  VIO_Volume  create_gradient_volume(
    VIO_Volume          volume,
    int             continuity,
    int             deriv_number )
{
    VIO_Volume           gradient_volume;
    int              volume_sizes[VIO_N_DIMENSIONS];
    int              x, y, z;
    VIO_Real             dx, dy, dz;
    VIO_Real             voxel[VIO_MAX_DIMENSIONS];
    VIO_Real             grad, value;
    VIO_Real             **first_deriv, ***second_deriv;
    VIO_Real             min_value, max_value;
    VIO_progress_struct  progress;

    gradient_volume = copy_volume_definition( volume, NC_FLOAT, FALSE,
                                              0.0, 0.0 );

    get_volume_sizes( volume, volume_sizes );

    if( deriv_number == 2 && continuity < 2 )
        continuity = 2;

    if( deriv_number == 1 )
    {
	printf( "Computing first derivative with continuity = %d\n", 
		continuity );
        VIO_ALLOC2D( first_deriv, 1, VIO_N_DIMENSIONS );
        second_deriv = NULL;
    }
    else
    {
	printf( "Computing second derivative with continuity = %d\n", 
		continuity );
        first_deriv = NULL;
        VIO_ALLOC3D( second_deriv, 1, VIO_N_DIMENSIONS, VIO_N_DIMENSIONS );
    }

    min_value = 0.0;
    max_value = 0.0;

    initialize_progress_report( &progress, FALSE,
                                volume_sizes[VIO_X] * volume_sizes[VIO_Y],
                                "Creating Gradient" );

    for_less( x, 0, volume_sizes[VIO_X] )
    {
        voxel[VIO_X] = (VIO_Real) x;
        for_less( y, 0, volume_sizes[VIO_Y] )
        {
            voxel[VIO_Y] = (VIO_Real) y;
            for_less( z, 0, volume_sizes[VIO_Z] )
            {
                voxel[VIO_Z] = (VIO_Real) z;
                evaluate_volume( volume, voxel, NULL, continuity,
                                 FALSE, 0.0, &value, first_deriv, second_deriv);

                if( deriv_number == 1 )
                {
                    dx = first_deriv[0][0];
                    dy = first_deriv[0][1];
                    dz = first_deriv[0][2];
                }
                else
                {
                    dx = second_deriv[0][0][0];
                    dy = second_deriv[0][1][1];
                    dz = second_deriv[0][2][2];
                }

                grad = sqrt(dx * dx + dy * dy + dz * dz);

                set_volume_real_value( gradient_volume, x, y, z, 0, 0, grad );

                if( x == 0 && y == 0 && z == 0 )
                {
                    min_value = grad;
                    max_value = grad;
                }
                else if( grad < min_value )
                    min_value = grad;
                else if( grad > max_value )
                    max_value = grad;
            }

            update_progress_report( &progress, x * volume_sizes[VIO_Y] + y + 1 );
        }
    }

    set_volume_real_range( gradient_volume, min_value - 1.0, max_value + 1.0 );

    print( "%g %g\n", min_value, max_value );

    terminate_progress_report( &progress );

    if( deriv_number == 1 )
    {
        VIO_FREE2D( first_deriv );
    }
    else
    {
        VIO_FREE3D( second_deriv );
    }

    return( gradient_volume );
}
