#include  <def_mni.h>
#include  <minc.h>

private  void  modify_volume(
    volume_struct  *volume,
    char           filename[],
    unsigned char  voxels[] );
private  void  render_marker_to_volume(
    volume_struct  *volume,
    unsigned char  voxels[],
    Real           render_size,
    marker_struct  *marker );

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *filename, *landmark_filename;
    Real                 render_size;
    volume_struct        volume;
    volume_input_struct  volume_input;
    unsigned char        *voxels;
    int                  n_objects;
    object_struct        **object_list;
    int                  i, sizes[N_DIMENSIONS], n_voxels;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &filename ) ||
        !get_real_argument( 0.0, &render_size ) )
    {
        print( "Need arguments.\n" );
        return( 1 );
    }

    status = start_volume_input( filename, FALSE, &volume, &volume_input );
    cancel_volume_input( &volume, &volume_input );

    get_volume_size( &volume, &sizes[X], &sizes[Y], &sizes[Z] );
    n_voxels = sizes[X] * sizes[Y] * sizes[Z];

    ALLOC( voxels, n_voxels );

    print( "Zeroing voxels.\n" );
    for_less( i, 0, n_voxels )
        voxels[i] = (unsigned char) 0;

    while( get_string_argument( "", &landmark_filename ) )
    {
        print( "Reading %s\n", landmark_filename );

        status = input_landmark_file( &volume, landmark_filename,
                                      GREEN, 1.0, BOX_MARKER, &n_objects,
                                      &object_list );

        if( status != OK )
            return( 1 );

        initialize_progress_report( &progress, FALSE, n_objects, "Voxelating" );

        for_less( i, 0, n_objects )
        {
            render_marker_to_volume( &volume, voxels, render_size,
                                     get_marker_ptr(object_list[i]) );
            update_progress_report( &progress, i+1 );
        }

        terminate_progress_report( &progress );

        delete_object_list( n_objects, object_list );
    }

    print( "Writing %s\n", filename );

    modify_volume( &volume, filename, voxels );

    return( status != OK );
}

private  void  modify_volume(
    volume_struct  *volume,
    char           filename[],
    unsigned char  voxels[] )
{
    int         sizes[N_DIMENSIONS];
    long        start[N_DIMENSIONS], count[N_DIMENSIONS];
    int         cdfid, err, img;

    start[X] = 0;
    start[Y] = 0;
    start[Z] = 0;
    get_volume_size( volume, &sizes[X], &sizes[Y], &sizes[Z] );
    count[X] = sizes[volume->axis_index_from_file[X]];
    count[Y] = sizes[volume->axis_index_from_file[Y]];
    count[Z] = sizes[volume->axis_index_from_file[Z]];

    cdfid = ncopen( filename, NC_WRITE );

    if( cdfid == MI_ERROR )
    {
        print( "cdfid error\n" );
        return;
    }

    img = ncvarid( cdfid, MIimage );

    err = mivarput( cdfid, img, start, count,
                    NC_BYTE, MI_UNSIGNED, (void *) voxels );

    if( err == MI_ERROR )
    {
        print( "mivarput error\n" );
        return;
    }

    err = ncclose( cdfid );

    if( err == MI_ERROR )
    {
        print( "ncclose error\n" );
        return;
    }
}

private  int  convert_index(
    int    axis_indices[],
    int    sizes[],
    int    x,
    int    y,
    int    z )
{
    int   indices[N_DIMENSIONS], file_indices[N_DIMENSIONS];

    indices[X] = x;
    indices[Y] = y;
    indices[Z] = z;

    file_indices[X] = indices[axis_indices[X]];
    file_indices[Y] = indices[axis_indices[Y]];
    file_indices[Z] = indices[axis_indices[Z]];

    return( IJK( file_indices[X], file_indices[Y], file_indices[Z],
                 sizes[axis_indices[Y]], sizes[axis_indices[Z]] ) );
}

private  void  render_marker_to_volume(
    volume_struct  *volume,
    unsigned char  voxels[],
    Real           render_size,
    marker_struct  *marker )
{
    int    ind;
    int    x, y, z, x_min, x_max, y_min, y_max, z_min, z_max;
    Real   x_voxel, y_voxel, z_voxel, dx, dy, dz;

    convert_world_to_voxel( volume,
                            Point_x(marker->position),
                            Point_y(marker->position),
                            Point_z(marker->position),
                            &x_voxel,
                            &y_voxel,
                            &z_voxel );

    x_min = (int) (x_voxel - render_size);
    x_max = (int) (x_voxel + render_size) + 1;
    y_min = (int) (y_voxel - render_size);
    y_max = (int) (y_voxel + render_size) + 1;
    z_min = (int) (z_voxel - render_size);
    z_max = (int) (z_voxel + render_size) + 1;

    for_inclusive( x, x_min, x_max )
    {
        dx = ((Real) x - x_voxel) * volume->thickness[X];
        for_inclusive( y, y_min, y_max )
        {
            dy = ((Real) y - y_voxel) * volume->thickness[Y];
            for_inclusive( z, z_min, z_max )
            {
                dz = ((Real) z - z_voxel) * volume->thickness[Z];

                if( dx * dx + dy * dy + dz * dz <= render_size * render_size &&
                    voxel_is_within_volume( volume, (Real) x, (Real) y,
                                            (Real) z ) )
                {
                    ind = convert_index( volume->axis_index_from_file,
                                         volume->sizes, x, y, z );

                    voxels[ind] = (unsigned char) marker->structure_id;
                }
            }
        }
    }
}
