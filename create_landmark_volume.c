#include  <def_mni.h>
#include  <minc.h>

private  void  print_ellipse_parameters( Point *, Point * );

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *input_filename, *landmark_filename, *output_filename;
    Real                 separations[N_DIMENSIONS];
    Real                 voxel[N_DIMENSIONS];
    Real                 min_voxel[N_DIMENSIONS];
    Real                 max_voxel[N_DIMENSIONS];
    Boolean              first;
    Point                min_point, max_point;
    Volume               volume, new_volume;
    volume_input_struct  volume_input;
    int                  n_objects;
    object_struct        **object_list;
    int                  i, c, x, y, z;
    int                  src_sizes[N_DIMENSIONS], dest_sizes[N_DIMENSIONS];
    int                  offset[N_DIMENSIONS];
    marker_struct        *marker;
    progress_struct      progress;
    static String        in_dim_names[] = { MIxspace, MIyspace, MIzspace };
    Transform            voxel_to_world_transform;
    Transform            translation;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &landmark_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Need arguments.\n" );
        return( 1 );
    }

    status = start_volume_input( input_filename, in_dim_names,
                                 FALSE, &volume, &volume_input );

    get_volume_sizes( volume, src_sizes );
    get_volume_separations( volume, separations );
    voxel_to_world_transform = volume->voxel_to_world_transform;

    if( filename_extension_matches( landmark_filename,
                                get_default_landmark_file_suffix() ) )
    {
        print( "Reading OLD format Landmark file %s\n", landmark_filename );

        status = input_landmark_file( volume, landmark_filename,
                                      GREEN, 1.0, BOX_MARKER,
                                      &n_objects, &object_list );
    }
    else
    {
        print( "Reading TAG file %s\n", landmark_filename );

        status = input_tag_file( landmark_filename,
                                 GREEN, 1.0, BOX_MARKER,
                                 &n_objects, &object_list );
    }

    if( status != OK )
        return( 1 );

    first = TRUE;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            convert_world_to_voxel( volume,
                                    Point_x(marker->position),
                                    Point_y(marker->position),
                                    Point_z(marker->position),
                                    &voxel[X], &voxel[Y], &voxel[Z] );

            if( voxel_is_within_volume( volume, voxel ) )
            {
                if( first )
                {
                    min_point = marker->position;
                    max_point = marker->position;
                    first = FALSE;
                }
                else
                {
                    apply_point_to_min_and_max( &marker->position,
                                                &min_point, &max_point );
                }
            }
            else
                print( "Landmark[%d] outside volume: %g %g %g\n", i,
                       voxel[X], voxel[Y], voxel[Z] );
        }
    }

    /* --- create the cropped volume */

    convert_world_to_voxel( volume, Point_x(min_point),
                            Point_y(min_point), Point_z(min_point),
                            &min_voxel[X], &min_voxel[Y], &min_voxel[Z] );
    convert_world_to_voxel( volume, Point_x(max_point),
                            Point_y(max_point), Point_z(max_point),
                            &max_voxel[X], &max_voxel[Y], &max_voxel[Z] );
    for_less( c, 0, N_DIMENSIONS )
    {
        offset[c] = MAX( 0, ROUND(min_voxel[c]) - 1 );
        dest_sizes[c] = MIN( src_sizes[c]-1, ROUND(max_voxel[c]) + 1 ) -
                        offset[c] + 1;
    }

    new_volume = create_volume( 3, in_dim_names, NC_BYTE, FALSE,
                                   0.0, 0.0 );
    set_volume_size( new_volume, NC_UNSPECIFIED, FALSE, dest_sizes );
    alloc_volume_data( new_volume );

    for_less( x, 0, dest_sizes[X] )
        for_less( y, 0, dest_sizes[Y] )
            for_less( z, 0, dest_sizes[Z] )
                SET_VOXEL_3D( new_volume, x, y, z, 0.0 );

    new_volume->min_voxel = 0.0;
    new_volume->max_voxel = 255.0;
    new_volume->value_scale = 1.0;
    new_volume->value_translation = 0.0;
    new_volume->separation[X] = separations[X];
    new_volume->separation[Y] = separations[Y];
    new_volume->separation[Z] = separations[Z];

    make_translation_transform( (Real) offset[X], (Real) offset[Y],
                                (Real) offset[Z], &translation );
    concat_transforms( &voxel_to_world_transform, &translation,
                       &voxel_to_world_transform );

    new_volume->voxel_to_world_transform = voxel_to_world_transform;

    initialize_progress_report( &progress, FALSE, n_objects, "Voxelating" );

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            convert_world_to_voxel( volume,
                                    Point_x(marker->position),
                                    Point_y(marker->position),
                                    Point_z(marker->position),
                                    &voxel[X], &voxel[Y], &voxel[Z] );

            voxel[X] -= (Real) offset[X];
            voxel[Y] -= (Real) offset[Y];
            voxel[Z] -= (Real) offset[Z];
            if( voxel_is_within_volume( new_volume, voxel ) )
            {
                SET_VOXEL_3D( new_volume, ROUND(voxel[X]),
                              ROUND(voxel[Y]), ROUND(voxel[Z]), 255.0 );
            }
        }

        update_progress_report( &progress, i+1 );
    }

    terminate_progress_report( &progress );

    cancel_volume_input( volume, &volume_input );

    delete_object_list( n_objects, object_list );

    print( "Writing %s\n", output_filename );

    status = output_volume( output_filename, 3, in_dim_names,
                            dest_sizes, NC_BYTE, FALSE, new_volume );

    print_ellipse_parameters( &min_point, &max_point );

    return( status != OK );
}

private  void  print_ellipse_parameters(
    Point   *min_point,
    Point   *max_point )
{
    int     c;
    Real    radius[N_DIMENSIONS];
    Point   centroid;

    INTERPOLATE_POINTS( centroid, *min_point, *max_point, 0.5 );

    for_less( c, 0, N_DIMENSIONS )
    {
        radius[c] = 1.0 * (Point_coord(*max_point,c) - Point_coord(centroid,c));
    }

    print( "%g %g %g %g %g %g\n",
           Point_x(centroid), Point_y(centroid), Point_z(centroid),
           radius[X], radius[Y], radius[Z] );
}
