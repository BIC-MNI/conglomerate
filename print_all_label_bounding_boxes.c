#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    static  VIO_STR  usage_str = "\n\
Usage: %s  labels.mnc [min_value max_value]\n\
\n\
     Displays a count of the number of voxels of the different labels in the\n\
     volume.\n\n";

    print_error( usage_str, executable );
}

typedef  struct
{
    int   count;
    VIO_Real  x_min;
    VIO_Real  y_min;
    VIO_Real  z_min;
    VIO_Real  x_max;
    VIO_Real  y_max;
    VIO_Real  z_max;
} label_info;

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename;
    VIO_Real                 min_volume, max_volume, voxel[VIO_N_DIMENSIONS];
    VIO_Real                 min_value, max_value, label, xw, yw, zw;
    int                  v0, v1, v2, v3, v4, n_labels, index;
    int                  i;
    label_info           *counts;
    VIO_Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_value );
    (void) get_real_argument( min_value - 1.0, &max_value );

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != VIO_OK )
        return( 1 );

    get_volume_real_range( volume, &min_volume, &max_volume );

    n_labels = VIO_ROUND( max_volume ) - VIO_ROUND( min_volume ) + 1;

    ALLOC( counts, n_labels );

    for_less( i, 0, n_labels )
        counts[i].count = 0;

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )
        label = get_volume_real_value( volume, v0, v1, v2, v3, v4 );
        index = VIO_ROUND(label) - VIO_ROUND(min_volume);

        voxel[0] = (VIO_Real) v0;
        voxel[1] = (VIO_Real) v1;
        voxel[2] = (VIO_Real) v2;
        convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );

        if( counts[index].count == 0 || xw < counts[index].x_min )
            counts[index].x_min = xw;
        if( counts[index].count == 0 || xw > counts[index].x_max )
            counts[index].x_max = xw;

        if( counts[index].count == 0 || yw < counts[index].y_min )
            counts[index].y_min = yw;
        if( counts[index].count == 0 || yw > counts[index].y_max )
            counts[index].y_max = yw;

        if( counts[index].count == 0 || zw < counts[index].z_min )
            counts[index].z_min = zw;
        if( counts[index].count == 0 || zw > counts[index].z_max )
            counts[index].z_max = zw;

        ++counts[index].count;
    END_ALL_VOXELS

    for_inclusive( i, VIO_ROUND(min_volume), VIO_ROUND(max_volume) )
    {
        index = i-VIO_ROUND(min_volume);
        if( i == 0 || counts[index].count == 0 )
            continue;

        print( "%3d    %6.0f   %6.0f   %6.0f   %6.0f   %6.0f   %6.0f\n",
                   i,
                   counts[index].x_min,
                   counts[index].x_max,
                   counts[index].y_min,
                   counts[index].y_max,
                   counts[index].z_min,
                   counts[index].z_max );
    }

    FREE( counts );

    return( 0 );
}
