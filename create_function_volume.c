#include  <volume_io.h>
#include  <bicpl.h>

private  VIO_Real  function(
    VIO_Real   x,
    VIO_Real   y,
    VIO_Real   z,
    VIO_Real   func_param );

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               output_filename;
    int                  i, x, y, z, sizes[N_DIMENSIONS];
    VIO_Real                 voxel[N_DIMENSIONS], world[N_DIMENSIONS];
    char                 history[10000];
    int                  n_values;
    VIO_Real                 min_value, max_value, value;
    VIO_Real                 x_min, x_max, y_min, y_max, z_min, z_max;
    VIO_Real                 func_param, separations[N_DIMENSIONS];
    VIO_Real                 x_world, y_world, z_world;
    nc_type              data_type;
    VIO_Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) )
    {
        print( "Need arguments.\n" );
        return( 1 );
    }

    (void) get_int_argument( 50, &sizes[X] );
    (void) get_int_argument( 50, &sizes[Y] );
    (void) get_int_argument( 50, &sizes[Z] );
    (void) get_real_argument( 0.0, &x_min );
    (void) get_real_argument( 1.0, &x_max );
    (void) get_real_argument( 0.0, &y_min );
    (void) get_real_argument( 1.0, &y_max );
    (void) get_real_argument( 0.0, &z_min );
    (void) get_real_argument( 1.0, &z_max );
    (void) get_real_argument( 0.0, &min_value );
    (void) get_real_argument( 1.0, &max_value );
    (void) get_real_argument( 0.0, &func_param );
    (void) get_int_argument( 4096, &n_values );

    if( n_values > 256 )
        data_type = NC_SHORT;
    else
        data_type = NC_BYTE;

    volume = create_volume( 3, XYZ_dimension_names, data_type, FALSE,
                            0.0, 0.0 );

    set_volume_sizes( volume, sizes );

    set_volume_real_range( volume, min_value, max_value );

    voxel[X] = -0.5;
    voxel[Y] = -0.5;
    voxel[Z] = -0.5;
    world[X] = x_min;
    world[Y] = y_min;
    world[Z] = z_min;

    set_volume_translation( volume, voxel, world );

    separations[X] = (x_max - x_min) / (VIO_Real) sizes[X];
    separations[Y] = (y_max - y_min) / (VIO_Real) sizes[Y];
    separations[Z] = (z_max - z_min) / (VIO_Real) sizes[Z];

    set_volume_separations( volume, separations );

    alloc_volume_data( volume );

    for_less( x, 0, sizes[X] )
    {
        x_world = x_min + separations[X] * ((VIO_Real) x + 0.5);
        for_less( y, 0, sizes[Y] )
        {
            y_world = y_min + separations[Y] * ((VIO_Real) y + 0.5);
            for_less( z, 0, sizes[Z] )
            {
                z_world = z_min + separations[Z] * ((VIO_Real) z + 0.5);

                value = function( x_world, y_world, z_world, func_param );

                if( value < min_value )
                    value = min_value;
                else if( value > max_value )
                    value = max_value;

                set_volume_real_value( volume, x, y, z, 0, 0, value );
            }
        }
    }

    separations[X] = 1.0;
    separations[Y] = 1.0;
    separations[Z] = 1.0;

    (void) strcpy( history, "Created by:  " );

    for_less( i, 0, argc )
    {
        (void) strcat( history, " " );
        (void) strcat( history, argv[i] );
    }
    (void) strcat( history, "\n" );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          volume, history, NULL );

    return( 0 );
}

private  VIO_Real  function(
    VIO_Real   x,
    VIO_Real   y,
    VIO_Real   z,
    VIO_Real   func_param )
{
    if( func_param == 0.0 )
        return( x );
    if( func_param == 1.0 )
        return( y );
    if( func_param == 2.0 )
        return( z );

    print( "Invalid func_param: %g\n", func_param );

    return( 0.0 );
}
