#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    Volume               volume;
    Real                 min_threshold, max_threshold, value;
    Real                 min_voxel, max_voxel;
    BOOLEAN              lower, change;
    int                  x, y, z, sizes[N_DIMENSIONS];
    int                  dx, dy, dz;
    int                  dx_min, dx_max, dy_min, dy_max, dz_min, dz_max;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &min_threshold ) ||
        !get_real_argument( 0.0, &max_threshold ) )
    {
        print( "%s  input.mnc  output.mnc  min max\n", argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    get_volume_voxel_range( volume, &min_voxel, &max_voxel );

    initialize_progress_report( &progress, FALSE, sizes[0] * sizes[1],
                                "Clamping" );

    for_less( x, 0, sizes[0] )
    {
        if( x == 0 )
            dx_min = 0;
        else
            dx_min = -1;
        if( x == sizes[0]-1 )
            dx_max = 0;
        else
            dx_max = 1;
        for_less( y, 0, sizes[1] )
        {
            if( y == 0 )
                dy_min = 0;
            else
                dy_min = -1;
            if( y == sizes[1]-1 )
                dy_max = 0;
            else
                dy_max = 1;
            for_less( z, 0, sizes[2] )
            {
                GET_VALUE_3D( value, volume, x, y, z );
                if( value >= min_threshold || value <= max_threshold )
                    continue;

                if( z == 0 )
                    dz_min = 0;
                else
                    dz_min = -1;
                if( z == sizes[2]-1 )
                    dz_max = 0;
                else
                    dz_max = 1;

                lower = (value < min_threshold);

                change = TRUE;
                for_inclusive( dx, dx_min, dx_max )
                {
                    for_inclusive( dy, dy_min, dy_max )
                    {
                        for_inclusive( dz, dz_min, dz_max )
                        {
                            GET_VALUE_3D( value, volume, x, y, z );
                            if( lower && value >= min_threshold ||
                                !lower && value <= max_threshold )
                            {
                                change = FALSE;
                                break;
                            }
                        }
                        if( !change ) break;
                    }
                    if( !change ) break;
                }

                if( change && lower )
                    SET_VOXEL_3D( volume, x, y, z, min_voxel )
                else if( change && !lower )
                    SET_VOXEL_3D( volume, x, y, z, max_voxel )
            }

            update_progress_report( &progress, x * sizes[1] + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, volume, input_filename,
                             "Clamped", (minc_output_options *) NULL );

    delete_volume( volume );

    return( 0 );
}
