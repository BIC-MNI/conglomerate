#include  <volume_io.h>

private  Volume   create_two_slice_volume(
    Volume   volume,
    int      axis );

private  void  usage(
    char   executable_name[] )
{
    print( "Usage: %s  input_volume  output_volume x|y|z  mm [mm] [mm] ...\n",
           executable_name );
}

int  main(
    int    argc,
    char   *argv[] )
{
    Status               status;
    int                  sizes[MAX_DIMENSIONS], new_sizes[MAX_DIMENSIONS];
    int                  a1, a2, axis, slice;
    int                  src_voxel[MAX_DIMENSIONS];
    int                  dest_voxel[MAX_DIMENSIONS];
    Real                 pos, voxel[MAX_DIMENSIONS], world[MAX_DIMENSIONS];
    Real                 dest_vox, src_vox;
    char                 *src_filename, *axis_name;
    char                 *dest_filename;
    Volume               volume, slice_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &dest_filename ) ||
        !get_string_argument( "", &axis_name ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    switch( axis_name[0] )
    {
    case 'x':
    case 'X':     axis = X;  break;

    case 'y':
    case 'Y':     axis = Y;  break;

    case 'z':
    case 'Z':     axis = Z;  break;

    default:
        usage( argv[0] );
        return( 1 );
    }

    /* read the input volume */

    if( input_volume( src_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE,
                      0.0, 0.0, TRUE, &volume, (minc_input_options *) NULL )
        != OK )
        return( 1 );

    if( get_volume_n_dimensions(volume) != 3 )
    {
        print( "Volume must be 3 dimensional.\n" );
        return( 1 );
    }

    slice_volume = create_two_slice_volume( volume, axis );

    get_volume_sizes( volume, sizes );
    get_volume_sizes( slice_volume, new_sizes );

    world[X] = 0.0;
    world[Y] = 0.0;
    world[Z] = 0.0;

    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    dest_voxel[axis] = 0;

    while( get_real_argument( 0.0, &pos ) )
    {
        world[axis] = pos;
        convert_world_to_voxel( volume, world[X], world[Y], world[Z], voxel );

        src_voxel[axis] = ROUND( voxel[axis] );

        for_less( src_voxel[a1], 0, new_sizes[a1] )
        {
            dest_voxel[a1] = src_voxel[a1];

            for_less( src_voxel[a2], 0, new_sizes[a2] )
            {
                dest_voxel[a2] = src_voxel[a2];

                GET_VOXEL_3D( dest_vox, slice_volume,
                              dest_voxel[X], dest_voxel[Y], dest_voxel[Z] );
                GET_VOXEL_3D( src_vox, volume, src_voxel[X], src_voxel[Y],
                              src_voxel[Z] );

                if( src_vox > dest_vox )
                    SET_VOXEL_3D( slice_volume, dest_voxel[X],
                                  dest_voxel[Y], dest_voxel[Z], src_vox );
            }
        }
    }

    for_less( dest_voxel[a1], 0, new_sizes[a1] )
    {
        for_less( dest_voxel[a2], 0, new_sizes[a2] )
        {
            for_less( slice, 0, new_sizes[axis] )
            { 
                dest_voxel[axis] = 0;
                GET_VOXEL_3D( dest_vox, slice_volume,
                              dest_voxel[X], dest_voxel[Y], dest_voxel[Z] );
                dest_voxel[axis] = slice;
                SET_VOXEL_3D( slice_volume, dest_voxel[X],
                              dest_voxel[Y], dest_voxel[Z], dest_vox );
            }
        }
    }

    status = output_volume( dest_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            slice_volume, "make_slice_volume:...",
                            (minc_output_options *) NULL );

    if( status != OK )
        print( "Unsuccessful.\n" );

    return( status != OK );
}

private  Volume   create_two_slice_volume(
    Volume   volume,
    int      axis )
{
    int                c, sizes[MAX_DIMENSIONS];
    Real               separations[MAX_DIMENSIONS];
    int                x, y, z, new_sizes[MAX_DIMENSIONS];
    Real               new_separations[MAX_DIMENSIONS];
    Volume             slice_volume;
    General_transform  *orig_transform, gen_convert_voxels, transform;
    Transform          convert_voxels;
    nc_type            nc_data_type;
    BOOLEAN            signed_flag;
    Real               voxel_min, voxel_max;

    get_volume_sizes( volume, sizes );
    get_volume_separations( volume, separations );

    nc_data_type = get_volume_nc_data_type( volume, &signed_flag );
    get_volume_voxel_range( volume, &voxel_min, &voxel_max );

    slice_volume = create_volume( get_volume_n_dimensions(volume),
                                  volume->dimension_names,
                                  nc_data_type, signed_flag,
                                  voxel_min, voxel_max );

    for_less( c, 0, get_volume_n_dimensions(volume) )
    {
        new_sizes[c] = sizes[c];
        new_separations[c] = separations[c];
    }

    new_sizes[axis] = 2;
    new_separations[axis] = separations[axis] * (Real) sizes[axis] /
                            (Real) new_sizes[axis];

    set_volume_sizes( slice_volume, new_sizes );
    alloc_volume_data( slice_volume );

    for_less( c, 0, N_DIMENSIONS )
        slice_volume->spatial_axes[c] = volume->spatial_axes[c];

    set_volume_real_range( slice_volume,
                           get_volume_real_min(volume),
                           get_volume_real_max(volume) );

    set_volume_separations( slice_volume, new_separations );

    for_less( c, 0, get_volume_n_dimensions(volume) )
        set_volume_direction_cosine( slice_volume, c,
                                     volume->direction_cosines[c] );

    orig_transform = get_voxel_to_world_transform( volume );

    make_identity_transform( &convert_voxels );
    Transform_elem( convert_voxels, axis, axis ) =
                     (Real) sizes[axis] / (Real) new_sizes[axis];
    Transform_elem( convert_voxels, axis, 3 ) =
                     (Real) sizes[axis] / 4.0 + 0.5;

    create_linear_transform( &gen_convert_voxels, &convert_voxels );
    concat_general_transforms( &gen_convert_voxels, orig_transform,
                               &transform );

    set_voxel_to_world_transform( slice_volume, &transform );

    delete_general_transform( &gen_convert_voxels );

    for_less( x, 0, new_sizes[X] )
    {
        for_less( y, 0, new_sizes[Y] )
        {
            for_less( z, 0, new_sizes[Z] )
            {
                SET_VOXEL_3D( slice_volume, x, y, z, voxel_min );
            }
        }
    }

    return( slice_volume );
}
