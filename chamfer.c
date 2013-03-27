#include  <bicpl.h>

int  main( argc, argv )
    int    argc;
    char   *argv[];
{
    VIO_Status            status;
    char              *input_filename, *output_filename;
    VIO_Real              threshold;
    VIO_Volume            volume;

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

    return( status != VIO_OK );
}

private  VIO_Volume  create_chamfer_volume(
    VIO_Volume   volume,
    VIO_Real     threshold )
{
    int      sizes[MAX_DIMENSIONS], ind[MAX_DIMENSIONS], neigh[MAX_DIMENSIONS];
    int      dir, c;
    VIO_Volume   chamfer;
    VIO_BOOL  surface_flag;
    VIO_Real     value, neigh_value;

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

                value = get_volume_real_value( volume, ind[X], ind[Y], ind[Z],
                                               0, 0 );

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
                            neigh_value = get_volume_real_value( volume,
                                          neigh[X], neigh[Y], neigh[Z], 0, 0 );

                            if( value <= threshold && neigh_value >= threshold||
                                value >= threshold && neigh_value <= threshold )
                            {
                                surface_flag = TRUE;
                            }
                        }

                        neigh[c] -= dir;
                    }
                }

                if( !surface_flag )
                {
                    set_volume_voxel_value( volume, ind[X], ind[Y], ind[Z],
                                            0, 0, 255.0 );
                }
            }
        }
    }

    return( chamfer );
}
