#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  N_OUTPUT  4

int   main(
    int   argc,
    char  *argv[] )
{
    char                  *output_prefix;
    char                  **input_filenames, *input_filename;
    char                  *dim_names_2d[2], *dim_names_3d[3];
    int                   n_input_files, d, i, vol, x_dim;
    int                   sizes_3d[3], sizes_2d[2];
    int                   v[5], n_slices;
    int                   slice, s;
    Real                  fraction_done, this_value, opposite_value;
    STRING                file_dim_names[N_DIMENSIONS];
    STRING                output_filename;
    char                  *suffixes[] = { "not_left_not_right",
                                          "not_left_right",
                                          "left_not_right",
                                          "left_right" };
    volume_input_struct   input_info;
    Real                  voxel;
    Volume                input_volume, output_volumes[N_OUTPUT];
    Minc_file             output_files[N_OUTPUT];
    General_transform     voxel_to_world_transform;
    minc_output_options   output_options;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_prefix ) )
    {
        print_error( "Usage: %s output_prefix input1.mnc input2.mnc ...\n",
                     argv[0] );
        return( 1 );
    }

    n_input_files = 0;

    while( get_string_argument( "", &input_filename ) )
    {
        ADD_ELEMENT_TO_ARRAY( input_filenames, n_input_files, input_filename,
                              DEFAULT_CHUNK_SIZE );
    }

    if( n_input_files == 0 )
    {
        print_error( "Need at least one input file.\n" );
        return( 1 );
    }

    if( get_file_dimension_names( input_filenames[0], N_DIMENSIONS,
                                  file_dim_names ) != OK )
    {
        print_error( "Error reading dimension names from %s.\n",
                     input_filenames[0] );
        return( 1 );
    }

    d = N_DIMENSIONS-1;

    while( strcmp( file_dim_names[d], MIxspace ) == 0 )
        --d;

    if( d == N_DIMENSIONS-1 )
        x_dim = 0;
    else
        x_dim = 1;

    ALLOC( dim_names_2d[x_dim], strlen(MIxspace) + 1 );
    (void) strcpy( dim_names_2d[x_dim], MIxspace );
    ALLOC( dim_names_2d[1-x_dim], strlen(file_dim_names[d]) + 1 );
    (void) strcpy( dim_names_2d[1-x_dim], file_dim_names[d] );

    /*--- get the 3d information about the file */

    for_less( d, 0, N_DIMENSIONS )
        dim_names_3d[d] = file_dim_names[d];

    if( start_volume_input( input_filenames[0], N_DIMENSIONS, dim_names_3d,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE,
                            &input_volume, NULL, &input_info ) != OK )
    {
        return( 1 );
    }

    get_volume_sizes( input_volume, sizes_3d );
    copy_general_transform( get_voxel_to_world_transform(input_volume),
                            &voxel_to_world_transform );

    delete_volume_input( &input_info );
    delete_volume( input_volume );

    /*--- get the 2d information */

    if( start_volume_input( input_filenames[0], 2, dim_names_2d,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE,
                            &input_volume, NULL, &input_info ) != OK )
    {
        return( 1 );
    }

    n_slices = get_n_input_volumes( input_info.minc_file );

    delete_volume_input( &input_info );

    for_less( i, 0, 2 )
    {
        for_less( d, 0, N_DIMENSIONS )
            if( strcmp( dim_names_2d[i], file_dim_names[d] ) == 0 )
                break;
        sizes_2d[i] = sizes_3d[d];
    }

    set_default_minc_output_options( &output_options );
    set_minc_output_real_range( &output_options, 0.0, 100.0 );

    for_less( i, 0, N_OUTPUT )
    {
        output_volumes[i] = create_volume( 2, dim_names_2d, NC_BYTE, FALSE,
                                           0.0, (Real) n_input_files );
        set_volume_real_range( output_volumes[i], 0.0, 100.0 );
        set_volume_sizes( output_volumes[i], sizes_2d );
        alloc_volume_data( output_volumes[i] );

        (void) sprintf( output_filename, "%s_%s.mnc", output_prefix,
                        suffixes[i] );

        output_files[i] = initialize_minc_output( output_filename, N_DIMENSIONS,
                                              file_dim_names, sizes_3d,
                                              NC_BYTE, FALSE,
                                              0.0, (Real) n_input_files,
                                              &voxel_to_world_transform,
                                              output_volumes[i],
                                              &output_options );

        if( output_files[i] == NULL )
            return( 1 );

        if( copy_auxiliary_data_from_minc_file( output_files[i],
                                                input_filenames[0], "" ) != OK )
            return( 1 );
    }

    for_less( slice, 0, n_slices )
    {
        /*--- initialize output volumes to 0 */

        BEGIN_ALL_VOXELS( output_volumes[0], v[0], v[1], v[2], v[3], v[4] )
            for_less( i, 0, N_OUTPUT )
                set_volume_real_value( output_volumes[i],
                                       v[0], v[1], v[2], v[3], v[4], 0.0 );
        END_ALL_VOXELS

        for_less( vol, 0, n_input_files )
        {
            if( start_volume_input( input_filenames[vol], 2, dim_names_2d,
                                    NC_UNSPECIFIED, FALSE, 0.0, 0.0, FALSE,
                                    &input_volume, NULL, &input_info ) != OK )
            {
                return( 1 );
            }

            for_less( s, 0, slice )
                (void) advance_input_volume( input_info.minc_file );

            while( input_more_of_volume( input_volume, &input_info,
                                         &fraction_done ) )
            {
            }

            BEGIN_ALL_VOXELS( output_volumes[0], v[0], v[1], v[2], v[3], v[4] )
                for_less( i, 0, N_OUTPUT )
                {
                    this_value = get_volume_real_value( input_volume,
                                                 v[0], v[1], v[2], v[3], v[4]);

                    v[x_dim] = sizes_2d[x_dim] - 1 - v[x_dim];

                    opposite_value = get_volume_real_value( input_volume,
                                                v[0], v[1], v[2], v[3], v[4]);

                    v[x_dim] = sizes_2d[x_dim] - 1 - v[x_dim];

                    if( (this_value > 0.0) == ((i & 2) != 0) &&
                        (opposite_value > 0.0) == ((i & 1) != 0) )
                    {
                        voxel = get_volume_voxel_value( output_volumes[i],
                                                  v[0], v[1], v[2], v[3], v[4]);
                        ++voxel;
                        set_volume_voxel_value( output_volumes[i],
                                                v[0], v[1], v[2], v[3], v[4],
                                                voxel );
                    }
                }
            END_ALL_VOXELS

            delete_volume_input( &input_info );
        }

        for_less( i, 0, N_OUTPUT )
        {
            if( output_minc_volume( output_files[i] ) != OK )
                return( 1 );
        }
    }

    for_less( i, 0, N_OUTPUT )
        (void) close_minc_output( output_files[i] );

    return( 0 );
}
