#include  <def_mni.h>

#define  X_MIN     40.0
#define  X_MAX     80.0
#define  Y_MIN    100.0
#define  Y_MAX    140.0
#define  Z_MIN     40.0
#define  Z_MAX     80.0

#define  NX   128
#define  NY   128
#define  NZ   128

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_filename  output_prefix\n", executable_name );
}

private  VIO_Volume  create_gradient_volume(
    VIO_Volume          volume,
    VIO_BOOL         value_range_present,
    int             min_value,
    int             max_value,
    Real            scaling );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_BOOL              value_range_present;
    int                  min_value, max_value;
    Real                 scaling;
    char                 *input_filename;
    char                 *output_prefix;
    int                  axis_index[3] = { Z, Y, X };
    VIO_Volume               volume, gradient_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_prefix ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( get_int_argument( 0, &min_value ) && get_int_argument( 0, &max_value ) )
        value_range_present = TRUE;
    else
        value_range_present = FALSE;

    (void) get_real_argument( 1.0, &scaling );

    /* read the input volume */

    if( input_volume( input_filename, &volume ) != VIO_OK )
        return( 1 );

    /* create the gradient volume */

    gradient_volume = create_gradient_volume( volume, value_range_present,
                                              min_value, max_value, scaling );

    /* output the gradient volume */

    (void) output_volume( output_prefix, gradient_volume, axis_index );

    return( 0 );
}

private  VIO_Volume  create_gradient_volume(
    VIO_Volume          volume,
    VIO_BOOL         value_range_present,
    int             min_value,
    int             max_value,
    Real            scaling )
{
    VIO_BOOL          within_range;
    int              sizes[N_DIMENSIONS], gradient_sizes[N_DIMENSIONS];
    Real             thickness[N_DIMENSIONS];
    Real             value;
    Real             xv, yv, zv;
    int              x, y, z;
    Real             dx, dy, dz;
    Real             grad;
    progress_struct  progress;
    volume_struct    *gradient_volume;

    get_volume_sizes( volume, sizes );
    get_volume_separations( volume, thickness );

    gradient_volume = create_volume( 3, (VIO_STR *) NULL, volume->nc_data_type,
                                     volume->signed_flag, 0.0, 0.0 );

    gradient_sizes[X] = NX;
    gradient_sizes[Y] = NY;
    gradient_sizes[Z] = NZ;

    set_volume_size( gradient_volume, NC_UNSPECIFIED, FALSE, gradient_sizes );

    gradient_volume->separation[X] = thickness[X] * (X_MAX - X_MIN) / (Real) NX;
    gradient_volume->separation[Y] = thickness[Y] * (Y_MAX - Y_MIN) / (Real) NY;
    gradient_volume->separation[Z] = thickness[Z] * (Z_MAX - Z_MIN) / (Real) NZ;

    initialize_progress_report( &progress, FALSE, NX * NY,
                                "Creating Gradient" );

    for_less( x, 0, NX )
    {
        xv = VIO_INTERPOLATE( ((Real) x + 0.5) / (Real) NX, X_MIN, X_MAX );
        for_less( y, 0, NY )
        {
            yv = VIO_INTERPOLATE( ((Real) y + 0.5) / (Real) NY, Y_MIN, Y_MAX );
            for_less( z, 0, NZ )
            {
                zv = VIO_INTERPOLATE( ((Real) z + 0.5) / (Real) NZ, Z_MIN, Z_MAX );

                (void) evaluate_volume( volume, xv, yv, zv, TRUE,
                                         &value, &dx, &dy, &dz );
                if( value_range_present )
                {
                    within_range = (min_value <= value && value <= max_value);
                }
                else
                    within_range = TRUE;

                if( within_range )
                {
                    grad = dx * dx + dy * dy + dz * dz;
                    if( grad != 0.0 )  grad = sqrt( grad );

                    grad *= scaling;

                    if( grad > 255.0 )
                        grad = 255.0;

                }
                else
                    grad = 0.0;

                SET_VOXEL_3D( gradient_volume, x, y, z, ROUND( grad ) );
            }

            update_progress_report( &progress, x * NY + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    return( gradient_volume );
}
