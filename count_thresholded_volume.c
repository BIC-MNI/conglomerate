#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *mask_volume_filename;
    Real                 mask_value, value;
    Real                 mask_voxel[MAX_DIMENSIONS];
    Real                 xw, yw, zw;
    Real                 min_threshold, max_threshold;
    Real                 separations[MAX_DIMENSIONS];
    int                  x, y, z, n_found;
    int                  mask_sizes[MAX_DIMENSIONS];
    progress_struct      progress;
    Volume               volume, mask_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &mask_volume_filename ) ||
        !get_string_argument( "", &volume_filename ) ||
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
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_volume( mask_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &mask_volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( mask_volume, mask_sizes );
    get_volume_separations( mask_volume, separations );

    initialize_progress_report( &progress, FALSE, mask_sizes[X] * mask_sizes[Y],
                                "Masking Volume" );

    n_found = 0;

    for_less( x, 0, mask_sizes[X] )
    {
        mask_voxel[X] = (Real) x;
        for_less( y, 0, mask_sizes[Y] )
        {
            mask_voxel[Y] = (Real) y;
            for_less( z, 0, mask_sizes[Z] )
            {
                mask_voxel[Z] = (Real) z;
                GET_VALUE_3D( mask_value, mask_volume, x, y, z );
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

            update_progress_report( &progress, x * mask_sizes[Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    print( "Number voxels: %d\n", n_found );
    print( "Volume       : %g cubic mm\n", (Real) n_found * separations[X] *
                                  separations[Y] * separations[Z] );

    return( 0 );
}
