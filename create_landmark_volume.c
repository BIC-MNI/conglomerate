#include  <def_mni.h>
#include  <minc.h>

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *input_filename, *landmark_filename, *output_filename;
    char                 *sphere_filename;
    Real                 separations[N_DIMENSIONS];
    Real                 voxel[N_DIMENSIONS];
    Volume               volume, output_volume;
    volume_input_struct  volume_input;
    int                  n_objects;
    object_struct        **object_list;
    int                  i, sizes[N_DIMENSIONS];
    marker_struct        *marker;
    progress_struct      progress;
    static String        out_dim_names[] = { MIxspace, MIyspace, MIzspace };
    Minc_file            minc_file;
    Transform            world_to_voxel_transform, voxel_to_world_transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &landmark_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Need arguments.\n" );
        return( 1 );
    }

    status = start_volume_input( input_filename, (String *) NULL,
                                 FALSE, &volume, &volume_input );

    get_volume_sizes( volume, sizes );
    get_volume_separations( volume, separations );
    world_to_voxel_transform = volume->world_to_voxel_transform;
    voxel_to_world_transform = volume->voxel_to_world_transform;

    cancel_volume_input( volume, &volume_input );

    output_volume = create_volume( 3, (String *) NULL, NC_BYTE, FALSE,
                                   0.0, 0.0 );
    set_volume_size( output_volume, NC_UNSPECIFIED, FALSE, sizes );
    alloc_volume_data( output_volume );

    output_volume->min_value = 0.0;
    output_volume->max_value = 1.0;
    output_volume->value_scale = 1.0;
    output_volume->value_translation = 0.0;
    output_volume->separation[X] = separations[X];
    output_volume->separation[Y] = separations[Y];
    output_volume->separation[Z] = separations[Z];
    output_volume->world_to_voxel_transform = world_to_voxel_transform;
    output_volume->voxel_to_world_transform = voxel_to_world_transform;

    print( "Reading %s\n", landmark_filename );

    status = input_landmark_file( output_volume, landmark_filename,
                                  GREEN, 1.0, BOX_MARKER, &n_objects,
                                  &object_list );

    if( status != OK )
        return( 1 );

    initialize_progress_report( &progress, FALSE, n_objects, "Voxelating" );

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            convert_world_to_voxel( output_volume,
                                    Point_x(marker->position),
                                    Point_y(marker->position),
                                    Point_z(marker->position),
                                    &voxel[X], &voxel[Y], &voxel[Z] );

            if( voxel_is_within_volume( output_volume, voxel ) )
            {
                SET_VOXEL_3D( output_volume, ROUND(voxel[X]),
                              ROUND(voxel[Y]), ROUND(voxel[Z]), 1.0 );
            }
            else
                print( "Landmark[%d] outside volume: %g %g %g\n", i,
                       voxel[X], voxel[Y], voxel[Z] );
        }

        update_progress_report( &progress, i+1 );
    }

    terminate_progress_report( &progress );

    delete_object_list( n_objects, object_list );

    print( "Writing %s\n", output_filename );

    minc_file = initialize_minc_output( output_filename, 3, out_dim_names,
                                    sizes, NC_BYTE, FALSE, 0.0, 1.0,
                                    &output_volume->voxel_to_world_transform );

    status = output_minc_volume( minc_file, output_volume );

    status = close_minc_output( minc_file );

    return( status != OK );
}
