#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  Real  compute_cross_correlation(
    Volume    volume1,
    Volume    volume2,
    Real      x,
    Real      super_sampling );

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
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
    STRING     input_filename1, input_filename2;
    Volume     volume1, volume2;
    Real       x, x_min, x_max, x_inc, value, super_sampling;

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
                      TRUE, &volume1, NULL ) != OK )
        return( 1 );

    if( input_volume( input_filename2, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume2, NULL ) != OK )
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

private  Real  compute_cross_correlation(
    Volume    volume1,
    Volume    volume2,
    Real      x,
    Real      super_sampling )
{
    Real  voxel[MAX_DIMENSIONS];
    int   sizes[MAX_DIMENSIONS], grid[MAX_DIMENSIONS];
    int   i, j, k, dim;
    Real  value1, value2, corr, xw, yw, zw, diff;

    corr = 0.0;

    get_volume_sizes( volume1, sizes );

    for_less( dim, 0, N_DIMENSIONS )
    {
        grid[dim] = ROUND( super_sampling * (Real) (sizes[dim] - 1) + 1.0 );
    }

    for_less( i, 0, grid[0] )
    {
        voxel[0] = INTERPOLATE( (Real) i / (Real) (grid[0]-1),
                                0.0, (Real) sizes[0] - 1.0 );

        for_less( j, 0, grid[1] )
        {
            voxel[1] = INTERPOLATE( (Real) j / (Real) (grid[1]-1),
                                    0.0, (Real) sizes[1] - 1.0 );

            for_less( k, 0, grid[2] )
            {
                voxel[2] = INTERPOLATE( (Real) k / (Real) (grid[2]-1),
                                        0.0, (Real) sizes[2] - 1.0 );

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

    corr /= (Real) (grid[0] * grid[1] * grid[2]);

    return( corr );
}
