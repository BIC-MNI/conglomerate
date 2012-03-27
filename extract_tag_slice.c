#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s  volume  output.tag  x|y|z  slice_mm  [min] [max]\n\
\n\
     Creates a tag file from a volume, taking only the tags from the given\n\
     slice.  An optional range of labels can be chosen to specify the tags\n\
     of interest.  Otherwise, all non-zero labels will be output.\n\n";

    print_error( usage_str, executable, executable, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename, tag_filename, axis_name;
    Volume               volume;
    FILE                 *file;
    Real                 min_id, max_id, world[N_DIMENSIONS];
    Real                 voxel[N_DIMENSIONS], slice_pos, value;
    Real                 tag[N_DIMENSIONS];
    int                  int_voxel[N_DIMENSIONS], sizes[N_DIMENSIONS];
    int                  n_tags, structure_id, a1, a2, axis;
    int                  block_sizes[MAX_DIMENSIONS];
    BOOLEAN              min_present;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &tag_filename ) ||
        !get_string_argument( NULL, &axis_name ) ||
        get_lower_case(axis_name[0]) < 'x' ||
        get_lower_case(axis_name[0]) > 'z' ||
        !get_real_argument( 0.0, &slice_pos ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    axis = get_lower_case(axis_name[0]) - 'x';

    min_present = get_real_argument( 0.0, &min_id );
    (void) get_real_argument( min_id, &max_id );

    if( min_present && min_id > max_id )
    {
        print( "min must be <= max\n" );
        return( 1 );
    }

    /*--- initialize volume for 1 slice caching for speed, since we
          only need 1 slice for this program */

    set_n_bytes_cache_threshold( 1 );
    set_default_max_bytes_in_cache( 1 );

    block_sizes[X] = -1;
    block_sizes[Y] = -1;
    block_sizes[Z] = -1;
    block_sizes[axis] = 1;
    set_default_cache_block_sizes( block_sizes );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    /* --- create the output tags */

    if( open_file_with_default_suffix( tag_filename,
                                       get_default_tag_file_suffix(),
                                       WRITE_FILE, ASCII_FORMAT,
                                       &file ) != OK ||
        initialize_tag_file_output( file,
                "Created by converting volume to tags.", 1 ) != OK )
    {
        return( 1 );
    }

    world[X] = 0.0;
    world[Y] = 0.0;
    world[Z] = 0.0;
    world[axis] = slice_pos;

    convert_world_to_voxel( volume, world[X], world[Y], world[Z], voxel );

    get_volume_sizes( volume, sizes );
    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;
    int_voxel[axis] = ROUND( voxel[axis] );

    if( int_voxel[axis] < 0 || int_voxel[axis] >= sizes[axis] )
    {
        print_error( "Slice position out of bounds of volume.\n" );
        return( 1 );
    }

    initialize_progress_report( &progress, FALSE, sizes[a1], "Creating tags" );

    n_tags = 0;
    for_less( int_voxel[a1], 0, sizes[a1] )
    {
        for_less( int_voxel[a2], 0, sizes[a2] )
        {
            value = get_volume_real_value( volume,
                          int_voxel[X], int_voxel[Y], int_voxel[Z], 0, 0 );

            if( !min_present && value != 0.0 ||
                min_present && min_id <= value && value <= max_id )
            {
                convert_3D_voxel_to_world( volume,
                                           (Real) int_voxel[X],
                                           (Real) int_voxel[Y],
                                           (Real) int_voxel[Z],
                                           &tag[X], &tag[Y], &tag[Z] );

                structure_id = ROUND( value );

                if( output_one_tag( file, 1, tag, NULL, NULL,
                                    &structure_id, NULL, NULL ) != OK )
                {
                    return( 1 );
                }

                ++n_tags;
            }
        }

        update_progress_report( &progress, int_voxel[a1] + 1 );
    }

    terminate_progress_report( &progress );

    terminate_tag_file_output( file );

    print( "Created %d tags.\n", n_tags );

    delete_volume( volume );

    return( 0 );
}
