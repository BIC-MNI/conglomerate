#include  <internal_volume_io.h>
#include  <bicpl.h>

private  Real  compute_cross_correlation(
    Volume    volume1,
    Volume    volume2,
    Real      x );

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
    Real       x, x_min, x_max, x_inc, value, diff;

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
        value = compute_cross_correlation( volume1, volume2, x );

        print( "%g %g\n", x, value );
    }

    delete_volume( volume1 );
    delete_volume( volume2 );

    return( 0 );
}

private  Real  compute_cross_correlation(
    Volume    volume1,
    Volume    volume2,
    Real      x )
{
    Real  v[MAX_DIMENSIONS];
    int   v0, v1, v2, v3, v4, sizes[MAX_DIMENSIONS];
    Real  value1, value2, corr, xw, yw, zw, diff;

    corr = 0.0;

    get_volume_sizes( volume1, sizes );

    BEGIN_ALL_VOXELS( volume1, v0, v1, v2, v3, v4 )

        value1 = get_volume_real_value( volume1, v0, v1, v2, v3, v4 );

        v[0] = (Real) v0;
        v[1] = (Real) v1;
        v[2] = (Real) v2;

        convert_voxel_to_world( volume1, v, &xw, &yw, &zw );
        xw += x;

        evaluate_volume_in_world( volume2, xw, yw, zw, 0, FALSE, 0.0,
                                  &value2, NULL, NULL, NULL,
                                           NULL, NULL, NULL,
                                           NULL, NULL, NULL );

        diff = value1 - value2;
        corr += diff * diff;

    END_ALL_VOXELS

    corr /= (Real) (sizes[0] * sizes[1] * sizes[2]);

    return( corr );
}
