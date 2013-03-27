#include  <volume_io.h>
#include  <bicpl.h>

private  VIO_Real  compute_cross_correlation(
    VIO_Volume    volume1,
    VIO_Volume    volume2,
    VIO_Real      x,
    VIO_Real      super_sampling );

private  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s input1.mnc input2.mnc  [x_min_offset]  [x_max_offset]  [x_increment]\n\
\n\
     Evaluates the cross correlation of the two volumes, with the set of\n\
     offsets.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR     input_filename1, input_filename2;
    VIO_Volume     volume1, volume2;
    VIO_Real       x, x_min, x_max, x_inc, value, super_sampling;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename1 ) ||
        !get_string_argument( "", &input_filename2 ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &x_min );
    (void) get_real_argument( 0.0, &x_max );
    (void) get_real_argument( 1.0, &x_inc );
    (void) get_real_argument( 1.0, &super_sampling );

    if( input_volume( input_filename1, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume1, NULL ) != VIO_OK )
        return( 1 );

    if( input_volume( input_filename2, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume2, NULL ) != VIO_OK )
        return( 1 );

    for( x = x_min;  x <= x_max;  x += x_inc )
    {
        value = compute_cross_correlation( volume1, volume2, x, super_sampling);

        print( "%g %g\n", x, value );
    }

    delete_volume( volume1 );
    delete_volume( volume2 );

    return( 0 );
}

private  VIO_Real  compute_cross_correlation(
    VIO_Volume    volume1,
    VIO_Volume    volume2,
    VIO_Real      x,
    VIO_Real      super_sampling )
{
    VIO_Real  voxel[MAX_DIMENSIONS];
    int   sizes[MAX_DIMENSIONS], grid[MAX_DIMENSIONS];
    int   i, j, k, dim;
    VIO_Real  value1, value2, corr, xw, yw, zw, diff;

    corr = 0.0;

    get_volume_sizes( volume1, sizes );

    for_less( dim, 0, N_DIMENSIONS )
    {
        grid[dim] = ROUND( super_sampling * (VIO_Real) (sizes[dim] - 1) + 1.0 );
    }

    for_less( i, 0, grid[0] )
    {
        voxel[0] = VIO_INTERPOLATE( (VIO_Real) i / (VIO_Real) (grid[0]-1),
                                0.0, (VIO_Real) sizes[0] - 1.0 );

        for_less( j, 0, grid[1] )
        {
            voxel[1] = VIO_INTERPOLATE( (VIO_Real) j / (VIO_Real) (grid[1]-1),
                                    0.0, (VIO_Real) sizes[1] - 1.0 );

            for_less( k, 0, grid[2] )
            {
                voxel[2] = VIO_INTERPOLATE( (VIO_Real) k / (VIO_Real) (grid[2]-1),
                                        0.0, (VIO_Real) sizes[2] - 1.0 );

                (void) evaluate_volume( volume1, voxel, NULL, 0, FALSE, 0.0,
                                        &value1, NULL, NULL );

                convert_voxel_to_world( volume1, voxel, &xw, &yw, &zw );
                xw += x;

                evaluate_volume_in_world( volume2, xw, yw, zw, 0, FALSE, 0.0,
                                          &value2, NULL, NULL, NULL,
                                                   NULL, NULL, NULL,
                                                   NULL, NULL, NULL );

                diff = value1 - value2;
                corr += diff * diff;
            }
        }
    }

    corr /= (VIO_Real) (grid[0] * grid[1] * grid[2]);

    return( corr );
}
