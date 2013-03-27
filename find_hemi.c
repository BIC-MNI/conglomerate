#include  <volume_io.h>
#include  <bicpl.h>

private  VIO_Real  compute_plane_value(
    VIO_Volume    volume,
    VIO_Real      x,
    VIO_Real      super_sampling );

private  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s input.mnc [xmin] [xmax] [step]\n\
\n\
     Finds the midplane.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR     input_filename;
    VIO_Volume     volume;
    VIO_Real       x, x_min, x_max, x_inc, value, super_sampling;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &x_min );
    (void) get_real_argument( 0.0, &x_max );
    (void) get_real_argument( 1.0, &x_inc );
    (void) get_real_argument( 1.0, &super_sampling );

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != VIO_OK )
        return( 1 );

    for( x = x_min;  x <= x_max;  x += x_inc )
    {
        value = compute_plane_value( volume, x, super_sampling );

        print( "%g %g\n", x, value );
    }

    delete_volume( volume );

    return( 0 );
}

private  VIO_Real  compute_plane_value(
    VIO_Volume    volume,
    VIO_Real      x,
    VIO_Real      super_sampling )
{
    VIO_Real  voxel[MAX_DIMENSIONS], xw, yw, zw;
    int   sizes[MAX_DIMENSIONS], y_grid, z_grid;
    int   i, j;
    VIO_Real  value, sum;

    get_volume_sizes( volume, sizes );

    y_grid = ROUND( super_sampling * (VIO_Real) (sizes[Y] - 1) + 1.0 );
    z_grid = ROUND( super_sampling * (VIO_Real) (sizes[Z] - 1) + 1.0 );

    sum = 0.0;

    voxel[X] = 0.0;

    for_less( i, 0, y_grid )
    {
        voxel[Y] = VIO_INTERPOLATE( (VIO_Real) i / (VIO_Real) (y_grid-1),
                                0.0, (VIO_Real) sizes[Y] - 1.0 );

        for_less( j, 0, z_grid )
        {
            voxel[Z] = VIO_INTERPOLATE( (VIO_Real) j / (VIO_Real) (z_grid-1),
                                    0.0, (VIO_Real) sizes[Z] - 1.0 );

            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            xw = x;

            evaluate_volume_in_world( volume, xw, yw, zw, 0, FALSE, 0.0,
                                      &value, NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL );


            sum += value * value;
        }
    }

    sum /= (VIO_Real) (y_grid * z_grid);

    return( sum );
}
