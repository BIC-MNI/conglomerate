#include  <mni.h>

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *volume_filename, *tag_filename;
    char                 *output_filename;
    Real                 voxel[MAX_DIMENSIONS];
    char                 history[10000];
    Volume               volume, new_volume;
    volume_input_struct  volume_input;
    int                  n_objects, structure_id;
    object_struct        **object_list;
    int                  i, x, y, z;
    int                  sizes[N_DIMENSIONS];
    marker_struct        *marker;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &tag_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "%s  example_volume  input.tag  output_file  [structure_id]\n",
               argv[0] );
        print( "\n" );
        print( "     Converts a tag file to a MINC volume, given an example MINC volume.\n" );
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

    if( input_objects_any_format( new_volume, tag_filename,
                                  GREEN, 1.0, BOX_MARKER,
                                  &n_objects, &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type(object_list[i]) == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            if( structure_id < 0 || structure_id == marker->structure_id )
            {
                convert_world_to_voxel( new_volume,
                                        Point_x(marker->position),
                                        Point_y(marker->position),
                                        Point_z(marker->position),
                                        voxel );

                if( voxel_is_within_volume( new_volume, voxel ) )
                {
                    x = ROUND( voxel[X] );
                    y = ROUND( voxel[Y] );
                    z = ROUND( voxel[Z] );
                    SET_VOXEL_3D( new_volume, x, y, z, marker->structure_id );
                }
            }
        }
    }

    delete_object_list( n_objects, object_list );

    (void) strcpy( history, "Created by:  " );

    for_less( i, 0, argc )
    {
        (void) strcat( history, " " );
        (void) strcat( history, argv[i] );
    }

    status = output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                            0.0, 0.0, new_volume, volume_filename, history,
                            (minc_output_options *) NULL );

    return( status != OK );
}
