#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: dilate_volume input.mnc output.mnc  d|e ratio\n\
                  [26/6] \n\
\n\
     Dilates or erodes in a continous fashion\n\n";

    print_error( usage_str, executable );
}

private  void  continuous_dilate_voxels_3d(
    Volume               input,
    Volume               output,
    BOOLEAN              dilating_flag,
    Neighbour_types      connectivity,
    Real                 ratio );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename, command;
    Real                 ratio;
    BOOLEAN              dilating;
    Volume               volume, dilated;
    int                  n_neighs;
    Neighbour_types      connectivity;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &command ) ||
        !get_real_argument( 0.0, &ratio ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    dilating = command[0] == 'd' || command[0] == 'D';

    (void) get_int_argument( 26, &n_neighs );

    switch( n_neighs )
    {
    case 6:   connectivity = FOUR_NEIGHBOURS;  break;
    case 26:   connectivity = EIGHT_NEIGHBOURS;  break;
    default:  print_error( "# neighs must be 6 or 26.\n" );  return( 1 );
    }

    set_n_bytes_cache_threshold( 1 );
    set_cache_block_sizes_hint( SLICE_ACCESS );
    set_default_max_bytes_in_cache( 1000000 );

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      NULL ) != OK )
        return( 1 );

    set_n_bytes_cache_threshold( 80000000 );

    dilated = copy_volume_definition( volume, NC_UNSPECIFIED, FALSE, 0.0, 0.0 );

    continuous_dilate_voxels_3d( volume, dilated, dilating, connectivity,
                                 ratio );

    delete_volume( volume );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, dilated, input_filename,
                                   "Smooth-Dilated\n", NULL );

    return( 0 );
}

private  void  continuous_dilate_voxels_3d(
    Volume               input,
    Volume               output,
    BOOLEAN              dilating_flag,
    Neighbour_types      connectivity,
    Real                 ratio )
{
    int               dir, n_dirs, *dx, *dy, *dz, sizes[N_DIMENSIONS];
    int               v0, v1, v2, t0, t1, t2;
    Real              value, original, extreme;
    BOOLEAN           found;
    progress_struct   progress;

    n_dirs = get_3D_neighbour_directions( connectivity, &dx, &dy, &dz );
    get_volume_sizes( input, sizes );

    initialize_progress_report( &progress, FALSE, sizes[0] * sizes[1],
                                "Smooth Dilating" );

    for_less( v0, 0, sizes[0] )
    for_less( v1, 0, sizes[1] )
    {
        for_less( v2, 0, sizes[2] )
        {
            extreme = 0.0;
            found = FALSE;
            for_less( dir, 0, n_dirs )
            {
                t0 = v0 + dx[dir];
                t1 = v1 + dy[dir];
                t2 = v2 + dz[dir];

                if( t0 >= 0 && t0 < sizes[0] &&
                    t1 >= 0 && t1 < sizes[1] &&
                    t2 >= 0 && t2 < sizes[2] )
                {
                    value = get_volume_real_value( input, t0, t1, t2, 0, 0 );
                    if( !found )
                    {
                        extreme = value;
                        found = TRUE;
                    }
                    else if( dilating_flag && value > extreme ||
                             !dilating_flag && value < extreme )
                         extreme = value;
                }
            }

            original = get_volume_real_value( input, v0, v1, v2, 0, 0 );
            value = INTERPOLATE( ratio, original, extreme );
            set_volume_real_value( output, v0, v1, v2, 0, 0, value );
        }

        update_progress_report( &progress, v0 * sizes[1] + v1 + 1 );
    }

    terminate_progress_report( &progress );
}
