#include  <mni.h>

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *input_volume_filename, *input_tags_filename;
    char                 *output_volume_filename;
    STRING               history;
    Volume               volume;
    int                  i, n_objects;
    Real                 value_to_set, voxel[N_DIMENSIONS];
    object_struct        **objects;
    marker_struct        *marker;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &input_tags_filename ) ||
        !get_string_argument( "", &output_volume_filename ) )
    {
        print( "Usage: %s  in_volume  in_tags  out_volume   [value_to_set]\n",
               argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &value_to_set );

    status = OK;

    status = input_volume( input_volume_filename, 3, XYZ_dimension_names,
                           NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                           TRUE, &volume, (minc_input_options *) NULL );

    if( status == OK )
        status =  status = input_objects_any_format( volume,
                                           input_tags_filename,
                                           GREEN, 1.0, BOX_MARKER,
                                           &n_objects, &objects );

    if( status == OK )
    {
        for_less( i, 0, n_objects )
        {
            if( objects[i]->object_type == MARKER )
            {
                marker = get_marker_ptr( objects[i] );

                convert_world_to_voxel( volume,
                                        Point_x(marker->position),
                                        Point_y(marker->position),
                                        Point_z(marker->position),
                                        voxel );

                if( voxel_is_within_volume( volume, voxel ) )
                {
                    SET_VOXEL_3D( volume,
                              ROUND(voxel[X]), ROUND(voxel[Y]), ROUND(voxel[Z]),
                              value_to_set );
                }
            }
        }

        delete_object_list( n_objects, objects );

        (void) strcpy( history, "Surface masked." );

        status = output_volume( output_volume_filename, NC_UNSPECIFIED, FALSE,
                                0.0, 0.0, volume, history,
                                (minc_output_options *) NULL );
    }

    return( status != OK );
}
