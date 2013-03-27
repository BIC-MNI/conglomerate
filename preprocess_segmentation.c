#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR   usage_str = "\n\
Usage: %s input.mnc output.mnc\n\
\n\
     Preprocesses classified volume for segmentation.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Real       voxel[VIO_N_DIMENSIONS], value;
    int        int_voxel[VIO_N_DIMENSIONS], iter, n_dilations;
    VIO_Volume     volume, label_volume;
    VIO_STR     input_filename, output_filename;
    int        range_changed[2][VIO_N_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 7, &n_dilations );

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume, NULL )
                      != VIO_OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_BYTE );

    convert_world_to_voxel( volume, -22.0, 10.0, 26.0, voxel );

    int_voxel[0] = VIO_ROUND( voxel[0] );
    int_voxel[1] = VIO_ROUND( voxel[1] );
    int_voxel[2] = VIO_ROUND( voxel[2] );

    do
    {
        int_voxel[VIO_Z] += 1;
        value = get_volume_real_value( volume, int_voxel[0], int_voxel[1],
                                       int_voxel[2], 0, 0 );
    }
    while( value != 3.0 );

    (void) fill_connected_voxels( volume, label_volume, EIGHT_NEIGHBOURS,
                                  int_voxel, 0, -1, 1, 3.0, 3.0,
                                  range_changed );

    for_less( iter, 0, n_dilations )
    {
        (void) dilate_voxels_3d( volume, label_volume, 1.0, 1.0, 0.0, -1.0,
                                 0.0, 0.0, 2.0, 2.0, 1.0, EIGHT_NEIGHBOURS,
                                 range_changed );
    }

    modify_labels_in_range( label_volume, volume, 0, -1, 0, 0.0, 0.5,
                            range_changed );

    (void) output_modified_volume( output_filename, NC_BYTE, FALSE,
                          0.0, 255.0, volume, input_filename,
                          "Preprocessed for segmentation.\n", NULL );

    return( 0 );
}
