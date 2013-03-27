#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename, mask_volume_filename;
    VIO_Real                 mask_value, value;
    VIO_Real                 mask_voxel[VIO_MAX_DIMENSIONS];
    VIO_Real                 xw, yw, zw;
    VIO_Real                 min_threshold, max_threshold;
    VIO_Real                 separations[VIO_MAX_DIMENSIONS];
    int                  x, y, z, n_found;
    int                  mask_sizes[VIO_MAX_DIMENSIONS];
    VIO_progress_struct      progress;
    VIO_Volume               volume, mask_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &mask_volume_filename ) ||
        !get_string_argument( NULL, &volume_filename ) ||
        !get_real_argument( 0.0, &min_threshold ) ||
        !get_real_argument( 0.0, &max_threshold ) )
    {
        print( "Usage: %s  mask_volume.mnc  volume.mnc  min_threshold  max_threshold\n",
               argv[0] );
        print( "Counts the number of non-zero voxels in mask_volume.mnc \n" );
        print( "that correspond to voxels in the volume.mnc within threshold.\n" );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != VIO_OK )
        return( 1 );

    if( equal_strings( volume_filename, mask_volume_filename ) )
        mask_volume = volume;
    else if( input_volume( mask_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &mask_volume, NULL ) != VIO_OK )
        return( 1 );

    get_volume_sizes( mask_volume, mask_sizes );
    get_volume_separations( mask_volume, separations );

    initialize_progress_report( &progress, FALSE, mask_sizes[VIO_X] * mask_sizes[VIO_Y],
                                "Masking VIO_Volume" );

    n_found = 0;

    for_less( x, 0, mask_sizes[VIO_X] )
    {
        mask_voxel[VIO_X] = (VIO_Real) x;
        for_less( y, 0, mask_sizes[VIO_Y] )
        {
            mask_voxel[VIO_Y] = (VIO_Real) y;
            for_less( z, 0, mask_sizes[VIO_Z] )
            {
                mask_voxel[VIO_Z] = (VIO_Real) z;
                mask_value = get_volume_real_value( mask_volume, x, y, z, 0, 0);
                if( mask_value != 0.0 )
                {
                    convert_voxel_to_world( mask_volume, mask_voxel,
                                            &xw, &yw, &zw );
                    evaluate_volume_in_world( volume, xw, yw, zw,
                                              0, FALSE,
                                              get_volume_real_min(volume),
                                              &value, NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL );

                    if( min_threshold <= value && value <= max_threshold )
                        ++n_found;
                }
            }

            update_progress_report( &progress, x * mask_sizes[VIO_Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    print( "Number voxels: %d\n", n_found );
    print( "VIO_Volume       : %g cubic mm\n", (VIO_Real) n_found * separations[VIO_X] *
                                  separations[VIO_Y] * separations[VIO_Z] );

    return( 0 );
}
