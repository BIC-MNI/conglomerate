#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    VIO_STR   executable )
{
    static  VIO_STR  usage_str = "\n\
Usage: %s  input.mnc  output.mnc  xc yc zc coef\n\
\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename;
    VIO_STR               output_filename;
    int                  v0, v1, v2, v3, v4;
    VIO_Real                 v[MAX_DIMENSIONS], xr, yr, zr;
    VIO_Volume               volume;
    VIO_Real                 xc, yc, zc, xw, yw, zw, d, coef, min_value, max_value;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &xc ) ||
        !get_real_argument( 0.0, &yc ) ||
        !get_real_argument( 0.0, &zc ) ||
        !get_real_argument( 0.0, &xr ) ||
        !get_real_argument( 0.0, &yr ) ||
        !get_real_argument( 0.0, &zr ) ||
        !get_real_argument( 0.0, &coef ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    min_value = 0.0;
    max_value = 100.0;
    set_volume_real_range( volume, min_value, max_value );

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )

        v[0] = (VIO_Real) v0;
        v[1] = (VIO_Real) v1;
        v[2] = (VIO_Real) v2;
        convert_voxel_to_world( volume, v, &xw, &yw, &zw );

        d = (xw - xc) * (xw - xc) / xr / xr +
            (yw - yc) * (yw - yc) / yr / yr +
            (zw - zc) * (zw - zc) / zr / zr;

        d *= coef;

        if( d < min_value )
            d = min_value;
        else if( d > max_value )
            d = max_value;

        set_volume_real_value( volume, v0, v1, v2, v3, v4, d );

    END_ALL_VOXELS

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, volume, "Core\n", NULL );

    return( 0 );
}
