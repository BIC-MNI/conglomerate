#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: tagtominc  example_volume.mnc  input.tag  output.mnc  [structure_id]\n\
\n\
     Converts a tag file to a MINC volume with values equal the tags'\n\
     structure ids, given an example MINC volume.\n\
     If structure_id is specified, then only tags with this id are used.\n\
     Otherwise all tags are used.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    STRING               volume_filename, tag_filename;
    STRING               output_filename;
    Real                 voxel[MAX_DIMENSIONS];
    STRING               history;
    Volume               volume, new_volume;
    volume_input_struct  volume_input;
    int                  structure_id, n_tag_points, n_volumes, *structure_ids;
    Real                 **tags1, **tags2;
    int                  i, x, y, z;
    int                  sizes[N_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &tag_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -1, &structure_id );

    if( start_volume_input( volume_filename, 3, XYZ_dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != OK )
        return( 1 );

    /* --- create the output volume */

    new_volume = copy_volume_definition( volume, NC_BYTE, FALSE, 0.0, 255.0 );

    cancel_volume_input( volume, &volume_input );

    get_volume_sizes( new_volume, sizes );
    set_volume_real_range( new_volume, 0.0, 255.0 );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                SET_VOXEL_3D( new_volume, x, y, z, 0.0 );
            }
        }
    }

    if( input_tag_file( tag_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, NULL, &structure_ids, NULL, NULL )
                        != OK )
        return( 1 );

    for_less( i, 0, n_tag_points )
    {
        if( structure_id < 0 || structure_id == structure_ids[i] )
        {
            convert_world_to_voxel( new_volume,
                                    tags1[i][X], tags1[i][Y], tags1[i][Z],
                                    voxel );

            if( voxel_is_within_volume( new_volume, voxel ) )
            {
                x = ROUND( voxel[X] );
                y = ROUND( voxel[Y] );
                z = ROUND( voxel[Z] );
                SET_VOXEL_3D( new_volume, x, y, z, structure_ids[i] );
            }
        }
    }

    free_tag_points( n_volumes, n_tag_points, tags1, tags2, NULL,
                     structure_ids, NULL, NULL );

    history = create_string( "Created by:  " );

    for_less( i, 0, argc )
    {
        concat_to_string( &history, " " );
        concat_to_string( &history, argv[i] );
    }

    status = output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                            0.0, 0.0, new_volume, volume_filename, history,
                            (minc_output_options *) NULL );

    return( status != OK );
}
