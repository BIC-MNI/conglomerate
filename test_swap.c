#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename, label_filename, dummy;
    STRING               dump_filename;
    Real                 mean, std_dev, value;
    Real                 min_sample_value, max_sample_value;
    Volume               volume, label_volume;
    Real                 separations[MAX_DIMENSIONS];
    Real                 min_world[MAX_DIMENSIONS], max_world[MAX_DIMENSIONS];
    Real                 min_voxel[MAX_DIMENSIONS], max_voxel[MAX_DIMENSIONS];
    Real                 real_v[MAX_DIMENSIONS], world[MAX_DIMENSIONS];
    Real                 min_value, max_value;
    FILE                 *file;
    BOOLEAN              first;
    int                  c, n_samples, v[MAX_DIMENSIONS], sizes[3];
    statistics_struct    stats;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &label_filename ) )
    {
        return( 1 );
    }

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    print( "Volume input\n" );
    flush_file( stdout );

    get_volume_separations( volume, separations );
    get_volume_real_range( volume, &min_value, &max_value );

    label_volume = create_label_volume( volume, NC_UNSPECIFIED );

    print( "Label volume created.\n" );
    flush_file( stdout );

    set_all_volume_label_data( label_volume, 0 );

    print( "Label volume initialized.\n" );
    flush_file( stdout );

    if( load_label_volume( label_filename, label_volume ) != OK )
        return( 1 );

    print( "Label volume loaded.\n" );
    flush_file( stdout );

    first = TRUE;

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        if( get_volume_label_data( label_volume, v ) != -1 )
        {
            value = get_volume_real_value( volume, v[0], v[1], v[2], v[3],v[4]);

            for_less( c, 0, N_DIMENSIONS )
                real_v[c] = (Real) v[c];

            convert_voxel_to_world( volume, real_v,
                                    &world[X], &world[Y], &world[Z]);

            if( first )
            {
                first = FALSE;
                for_less( c, 0, N_DIMENSIONS )
                {
                    min_voxel[c] = real_v[c];
                    max_voxel[c] = real_v[c];
                    min_world[c] = world[c];
                    max_world[c] = world[c];
                }
            }
            else
            {
                for_less( c, 0, N_DIMENSIONS )
                {
                    if( real_v[c] < min_voxel[c] )
                        min_voxel[c] = real_v[c];
                    else if( real_v[c] > max_voxel[c] )
                        max_voxel[c] = real_v[c];

                    if( world[c] < min_world[c] )
                        min_world[c] = world[c];
                    else if( world[c] > max_world[c] )
                        max_world[c] = world[c];
                }
            }
        }

    END_ALL_VOXELS

    print( "Done\n" );
    flush_file( stdout );

    return( 0 );
}
