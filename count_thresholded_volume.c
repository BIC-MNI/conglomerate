#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *mask_volume_filename;
    Real                 mask_value, value;
    Real                 threshold, separations[MAX_DIMENSIONS];
    int                  i, x, y, z, sizes[MAX_DIMENSIONS], n_found;
    int                  mask_sizes[MAX_DIMENSIONS];
    progress_struct      progress;
    Volume               volume, mask_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &mask_volume_filename ) ||
        !get_string_argument( "", &volume_filename ) ||
        !get_real_argument( 0.0, &threshold ) )
    {
        print( "Usage: %s  mask_volume.mnc  volume.mnc  threshold\n",
               argv[0] );
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

    get_volume_sizes( volume, sizes );
    get_volume_sizes( mask_volume, mask_sizes );
    get_volume_separations( volume, separations );

    for_less( i, 0, N_DIMENSIONS )
    {
        if( sizes[i] != mask_sizes[i] )
        {
            print( "Mask volume and volume sizes do not match.\n" );
            return( 1 );
        }
    }

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Masking Volume" );

    n_found = 0;

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VALUE_3D( mask_value, mask_volume, x, y, z );
                if( mask_value != 0.0 )
                {
                    GET_VALUE_3D( value, volume, x, y, z );
                    if( value >= threshold )
                        ++n_found;
                }
            }

            update_progress_report( &progress, x * sizes[Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    print( "Number voxels: %d\n", n_found );
    print( "Volume       : %g cubic mm\n", (Real) n_found * separations[X] *
                                  separations[Y] * separations[Z] );

    return( 0 );
}
