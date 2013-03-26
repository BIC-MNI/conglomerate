#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               input_volume_filename, input_surface_filename;
    VIO_STR               output_volume_filename;
    Real                 separations[VIO_MAX_DIMENSIONS];
    Real                 voxel_size, threshold;
    VIO_STR               history;
    VIO_File_formats         format;
    VIO_Volume               volume, label_volume;
    int                  n_objects, voxel[VIO_MAX_DIMENSIONS];
    int                  sizes[VIO_MAX_DIMENSIONS], n_changed;
    int                  range_changed[2][VIO_N_DIMENSIONS];
    object_struct        **objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &input_surface_filename ) ||
        !get_real_argument( 0.0, &threshold ) ||
        !get_string_argument( NULL, &output_volume_filename ) )
    {
        print( "Usage: %s  in_volume.mnc  in_surface.obj  threshold out_volume.mnc\n",
               argv[0] );
        return( 1 );
    }

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_BYTE );

    if( input_graphics_file( input_surface_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects < 1 )
    {
        print( "No objects in %s.\n", input_surface_filename);
        return( 1 );
    }

    get_volume_separations( volume, separations );
    get_volume_sizes( volume, sizes );

    voxel_size = MIN3( VIO_FABS(separations[0]), VIO_FABS(separations[1]),
                       VIO_FABS(separations[2]) );

    print( "Scanning object to volume.\n" );

    scan_object_to_volume( objects[0], volume, label_volume, 1,
                           voxel_size / 2.0 );

    (void) dilate_voxels_3d( volume, label_volume,
                             1.0, 1.0, 0.0, -1.0, 
                             0.0, 0.0, 0.0, -1.0,
                             1.0, EIGHT_NEIGHBOURS, range_changed );

    voxel[0] = 0;
    voxel[1] = 0;
    voxel[2] = 0;
    fill_connected_voxels( volume, label_volume, EIGHT_NEIGHBOURS,
                           voxel, 0, 0, 2, 0.0, -1.0, range_changed );

    modify_labels_in_range( volume, label_volume, 1, 1, 0, 0.0, -1.0,
                            range_changed );

    n_changed = dilate_voxels_3d( volume, label_volume,
                                  2.0, 2.0, 0.0, -1.0, 
                                  0.0, 0.0, 0.0, threshold,
                                  2.0, EIGHT_NEIGHBOURS, range_changed );
    print( "Dilated: %d\n", n_changed );

    n_changed = dilate_voxels_3d( volume, label_volume,
                                  2.0, 2.0, 0.0, -1.0, 
                                  0.0, 0.0, 0.0, threshold,
                                  2.0, EIGHT_NEIGHBOURS, range_changed );
    print( "Dilated: %d\n", n_changed );

/*
    do
    {
        n_changed = dilate_voxels_3d( volume, label_volume,
                                      2.0, 2.0, 0.0, -1.0, 
                                      0.0, 0.0, threshold, 1.0e30,
                                      2.0, FOUR_NEIGHBOURS, range_changed );
        print( "N changed: %d\n", n_changed );
    }
    while( n_changed > 0 );
*/

    history = create_string( "fill_sulci ...\n" );

    (void) output_volume( output_volume_filename, NC_UNSPECIFIED,
                          FALSE, 0.0, 0.0, label_volume, history,
                          NULL );

    delete_string( history );

    delete_object_list( n_objects, objects );
    delete_volume( volume );
    delete_volume( label_volume );

    return( 0 );
}
