#include  <bicpl.h>

int  main( argc, argv )
    int    argc;
    char   *argv[];
{
    Status            status;
    char              *input_filename, *output_filename;
    Real              threshold;
    Volume            volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &isovalue ) )
    {
        print( "Usage:  chamfer  input_filename  output_filename  threshold\n");
        return( 1 );
    }

    status = input_volume( input_filename, 3, XYZ_dimension_names,
                           NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE,
                           &volume, (minc_input_options *) NULL );

    new_volume = create_chamfer_volume( volume, threshold );

    status = output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                            0.0, 0.0, new_volume, "chamfered",
                            (minc_output_options *) NULL );

    return( status != OK );
}

private  Volume  create_chamfer_volume(
    Volume   volume,
    Real     threshold )
{
    int      sizes[MAX_DIMENSIONS], ind[MAX_DIMENSIONS], neigh[MAX_DIMENSIONS];
    int      dir, c;
    Volume   chamfer;
    BOOLEAN  surface_flag;
    Real     voxel, value, neigh_voxel, neigh_value;

    chamfer = copy_volume_definition( volume, NC_UNSPECIFIED, FALSE, 0.0, 0.0 );

    get_volume_sizes( volume, sizes );

    for_less( ind[X], 0, sizes[X] )
    {
        for_less( ind[Y], 0, sizes[Y] )
        {
            for_less( ind[Z], 0, sizes[Z] )
            {
                surface_flag = FALSE;
                min_diff = 0.0;

                GET_VOXEL_3D( voxel, volume, ind[X], ind[Y], ind[Z] );
                value = CONVERT_VOXEL_TO_VALUE( volume, voxel );

                neigh[X] = ind[X];
                neigh[Y] = ind[Y];
                neigh[Z] = ind[Z];

                for( dir = -1;  dir <= 1;  dir += 2 )
                {
                    for_less( c, 0, N_DIMENSIONS )
                    {
                        neigh[c] += dir;

                        if( neigh[c] >= 0 && neigh[c] < sizes[c] )
                        {
                            GET_VOXEL_3D( neigh_voxel, volume,
                                          neigh[X], neigh[Y], neigh[Z] );
                            neigh_value = CONVERT_VOXEL_TO_VALUE( volume,
                                                                  neigh_voxel );

                            if( value <= threshold && neigh_voxel >= threshold||
                                value >= threshold && neigh_voxel <= threshold )
                            {
                                surface_flag = TRUE;
                            }
                        }

                        neigh[c] -= dir;
                    }
                }

                if( !surface_flag )
                {
                    SET_VOXEL_3D( volume, ind[X], ind[Y], ind[Z], 255.0 );
                }
            }
        }
    }

    return( chamfer );
}
