#include  <bicpl.h>
#include  <internal_volume_io.h>

private  void  usage(
    STRING   executable_name )
{
    STRING  usage_str = "\n\
Usage: %s  input.mnc  output.mnc [1|2] [1|3]\n\
\n\
     Creates a magnitude of gradient or second derivative of the volume,\n\
     depending on whether 1 or 2 is specified.  The choice of 1 or 3 selects\n\
     linear or cubic interpolation.  If the second derivative is chosen,\n\
     cubic interpolation will be used.\n\n";

     print_error( usage_str, executable_name );

}

private  Volume  create_gradient_volume(
    Volume          volume,
    int             continuity,
    int             deriv_number );

int  main(
    int    argc,
    char   *argv[] )
{
    int                  deriv_number, order;
    STRING               input_filename;
    STRING               output_filename;
    STRING               history;
    Volume               volume, gradient_volume;

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
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    /* --- create the gradient volume */

    gradient_volume = create_gradient_volume( volume, deriv_number, order - 1 );

    history = create_string( "make_gradient_volume\n" );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                           0.0, 0.0, gradient_volume, input_filename,
                           history, (minc_output_options *) NULL );


    return( 0 );
}

private  Volume  create_gradient_volume(
    Volume          volume,
    int             continuity,
    int             deriv_number )
{
    Volume           gradient_volume;
    int              volume_sizes[N_DIMENSIONS];
    int              x, y, z, dx, dy, dz;
    Real             voxel[MAX_DIMENSIONS];
    Real             grad, value;
    Real             **first_deriv, ***second_deriv;
    Real             min_value, max_value;
    progress_struct  progress;

    gradient_volume = copy_volume_definition( volume, NC_FLOAT, FALSE,
                                              0.0, 0.0 );

    get_volume_sizes( volume, volume_sizes );

    if( deriv_number == 2 && continuity < 2 )
        continuity = 2;

    if( deriv_number == 1 )
    {
        ALLOC2D( first_deriv, 1, N_DIMENSIONS );
        second_deriv = NULL;
    }
    else
    {
        first_deriv = NULL;
        ALLOC3D( second_deriv, 1, N_DIMENSIONS, N_DIMENSIONS );
    }

    min_value = 0.0;
    max_value = 0.0;

    initialize_progress_report( &progress, FALSE,
                                volume_sizes[X] * volume_sizes[Y],
                                "Creating Gradient" );

    for_less( x, 0, volume_sizes[X] )
    {
        voxel[X] = (Real) x;
        for_less( y, 0, volume_sizes[Y] )
        {
            voxel[Y] = (Real) y;
            for_less( z, 0, volume_sizes[Z] )
            {
                voxel[Z] = (Real) z;
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

                grad = dx * dx + dy * dy + dz * dz;

                set_volume_real_value( gradient_volume, x, y, z, 0, 0, grad );

                if( x == 0 && y == 0 && z == 0 )
                {
                    min_value = value;
                    max_value = value;
                }
                else if( value < min_value )
                    min_value = value;
                else if( value > max_value )
                    max_value = value;
            }

            update_progress_report( &progress, x * volume_sizes[Y] + y + 1 );
        }
    }

    set_volume_real_range( gradient_volume, min_value - 1.0, max_value + 1.0 );

    print( "%g %g\n", min_value, max_value );

    terminate_progress_report( &progress );

    if( deriv_number == 1 )
    {
        FREE2D( first_deriv );
    }
    else
    {
        FREE3D( second_deriv );
    }

    return( gradient_volume );
}
