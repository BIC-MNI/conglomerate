#include  <volume_io.h>
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
    Real                 **input[3],
    Volume               output,
    int                  slice,
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
    Volume               volume, dilated, input_3d;
    int                  n_neighs, slice, n_slices, first_slice;
    int                  sizes_2d[2], v0, v1, v2;
    Neighbour_types      connectivity;
    static char          *dim_names[] = { "", "" };
    Minc_file            minc_file;
    Real                 **slices[3], **swap, amount_done;
    progress_struct      progress;

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

    if( input_volume_header_only( input_filename, 3,
                       File_order_dimension_names, &input_3d, NULL ) != OK )
        return( 1 );

    set_n_bytes_cache_threshold( 1 );
    set_default_max_bytes_in_cache( 1 );
    set_cache_block_sizes_hint( SLICE_ACCESS );

    dilated = copy_volume_definition( input_3d,
                                      NC_UNSPECIFIED, FALSE, 0.0, 0.0 );

    set_cache_output_volume_parameters( dilated, output_filename,
                                        NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                                        input_filename,
                                        "Smooth-Dilated\n", NULL );

    delete_volume( input_3d );

    set_n_bytes_cache_threshold( 80000000 );
    set_default_max_bytes_in_cache( 80000000 );

    volume = create_volume( 2, dim_names, NC_UNSPECIFIED, FALSE, 0.0, 0.0 );

    minc_file = initialize_minc_input( input_filename, volume, NULL );

    if( minc_file == NULL )
        return( 1 );

    n_slices = get_n_input_volumes( minc_file );
    get_volume_sizes( volume, sizes_2d );

    for_less( v0, 0, 3 )
    {
        ALLOC2D( slices[v0], sizes_2d[0], sizes_2d[1] );

        for_less( v1, 0, sizes_2d[0] )
        for_less( v2, 0, sizes_2d[1] )
            slices[v0][v1][v2] = 0.0;
    }

    first_slice = -3;

    initialize_progress_report( &progress, FALSE, n_slices, "Smooth Dilating" );

    for_less( slice, 0, n_slices )
    {
        while( first_slice < slice-1 )
        {
            swap = slices[0];
            slices[0] = slices[1];
            slices[1] = slices[2];
            slices[2] = swap;
            ++first_slice;

            if( first_slice + 2 < n_slices )
            {
                while( input_more_minc_file( minc_file, &amount_done ) )
                {}
                (void) advance_input_volume( minc_file );

                get_volume_value_hyperslab_2d( volume, 0, 0,
                                               sizes_2d[0], sizes_2d[1],
                                               &slices[2][0][0] );
            }
            else
            {
                for_less( v1, 0, sizes_2d[0] )
                for_less( v2, 0, sizes_2d[1] )
                    slices[2][v1][v2] = 0.0;
            }
        }

        continuous_dilate_voxels_3d( slices, dilated, slice,
                                     dilating, connectivity, ratio );

        update_progress_report( &progress, slice + 1 );
    }

    terminate_progress_report( &progress );

    delete_volume( dilated );

    return( 0 );
}

private  void  continuous_dilate_voxels_3d(
    Real                 **input[3],
    Volume               output,
    int                  slice,
    BOOLEAN              dilating_flag,
    Neighbour_types      connectivity,
    Real                 ratio )
{
    int               dir, n_dirs, *dx, *dy, *dz, sizes[N_DIMENSIONS];
    int               v1, v2, t0, t1, t2;
    Real              value, original, extreme;

    n_dirs = get_3D_neighbour_directions( connectivity, &dx, &dy, &dz );
    get_volume_sizes( output, sizes );

    for_less( v1, 0, sizes[1] )
    for_less( v2, 0, sizes[2] )
    {
        extreme = 0.0;
        for_less( dir, 0, n_dirs )
        {
            t0 = dx[dir] + 1;
            t1 = v1 + dy[dir];
            t2 = v2 + dz[dir];

            if( t1 >= 0 && t1 < sizes[1] &&
                t2 >= 0 && t2 < sizes[2] )
            {
                value = input[t0][t1][t2];
            }
            else
                value = 0.0;

            if( dir == 0 ||
                dilating_flag && value > extreme ||
                !dilating_flag && value < extreme )
            {
                 extreme = value;
            }
        }

        original = input[1][v1][v2];
        if( dilating_flag && extreme > original ||
            !dilating_flag && extreme < original )
        {
            value = INTERPOLATE( ratio, original, extreme );
        }
        else
            value = original;

        set_volume_real_value( output, slice, v1, v2, 0, 0, value );
    }
}
