#include  <internal_volume_io.h>
#include  <bicpl.h>

private  Real  compute_plane_value(
    Volume    volume,
    Real      x,
    Real      super_sampling );

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s input.mnc [xmin] [xmax] [step]\n\
\n\
     Finds the midplane.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING     input_filename;
    Volume     volume;
    Real       x, x_min, x_max, x_inc, value, super_sampling;

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
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    for( x = x_min;  x <= x_max;  x += x_inc )
    {
        value = compute_plane_value( volume, x, super_sampling );

        print( "%g %g\n", x, value );
    }

    delete_volume( volume );

    return( 0 );
}

private  Real  compute_plane_value(
    Volume    volume,
    Real      x,
    Real      super_sampling )
{
    Real  voxel[MAX_DIMENSIONS], xw, yw, zw;
    int   sizes[MAX_DIMENSIONS], y_grid, z_grid;
    int   i, j;
    Real  value, sum;

    get_volume_sizes( volume, sizes );

    y_grid = ROUND( super_sampling * (Real) (sizes[Y] - 1) + 1.0 );
    z_grid = ROUND( super_sampling * (Real) (sizes[Z] - 1) + 1.0 );

    sum = 0.0;

    voxel[X] = 0.0;

    for_less( i, 0, y_grid )
    {
        voxel[Y] = INTERPOLATE( (Real) i / (Real) (y_grid-1),
                                0.0, (Real) sizes[Y] - 1.0 );

        for_less( j, 0, z_grid )
        {
            voxel[Z] = INTERPOLATE( (Real) j / (Real) (z_grid-1),
                                    0.0, (Real) sizes[Z] - 1.0 );

            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            xw = x;

            evaluate_volume_in_world( volume, xw, yw, zw, 0, FALSE, 0.0,
                                      &value, NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL );


            sum += value * value;
        }
    }

    sum /= (Real) (y_grid * z_grid);

    return( sum );
}
