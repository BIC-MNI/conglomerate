#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s input.mnc [threshold]\n\
\n\
     Finds the centroid of the volume.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR     input_filename;
    VIO_Volume     volume;
    int        v[VIO_MAX_DIMENSIONS], n_dims, dim;
    VIO_Real       value, weight, threshold, xw, yw, zw;
    VIO_Real       voxel_centroid[VIO_MAX_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &threshold );

    if( input_volume( input_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != VIO_OK )
        return( 1 );

    n_dims = get_volume_n_dimensions( volume );

    for_less( dim, 0, n_dims )
        voxel_centroid[dim] = 0.0;
    weight = 0.0;

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        value = get_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4] );

        if( value >= threshold )
        {
            weight += value;
            for_less( dim, 0, n_dims )
                voxel_centroid[dim] += (VIO_Real) v[dim] * value;
        }

    END_ALL_VOXELS

    for_less( dim, 0, n_dims )
        voxel_centroid[dim] /= weight;

    convert_voxel_to_world( volume, voxel_centroid, &xw, &yw, &zw );

    print( "Centroid: %g %g %g\n", xw, yw, zw );

    delete_volume( volume );

    return( 0 );
}
