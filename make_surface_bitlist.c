
#include  <volume_io.h>
#include  <bicpl.h>

static  void  label_surface_voxels(
    VIO_Volume               volume,
    bitlist_3d_struct    *bitlist,
    int                  bitlist_sizes[],
    VIO_General_transform    *bitlist_to_world,
    VIO_Real                 threshold,
    VIO_Real                 tolerance );

static  VIO_BOOL  volume_range_contains_threshold(
    VIO_Volume   volume,
    VIO_Real     min_voxel[],
    VIO_Real     max_voxel[],
    VIO_Real     threshold );

static  void  usage(
    char   executable[] )
{
    VIO_STR   usage_str = "\n\
Usage: %s   input.mnc  output_prefix   threshold  \n\
            [x_size]  [y_size]  [z_size]  [tolerance]\n\
\n\
     Creates the surface bitlist volume.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR              input_filename, transform_filename, bitlist_filename;
    VIO_STR              output_prefix;
    VIO_Volume              volume;
    bitlist_3d_struct   bitlist;
    FILE                *file;
    int                 dim;
    int                 sizes[VIO_N_DIMENSIONS], bitlist_sizes[VIO_N_DIMENSIONS];
    VIO_Real                threshold;
    VIO_Real                tolerance;
    VIO_Transform           new_to_old;
    VIO_General_transform   *volume_transform, mod_transform, new_transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_prefix ) ||
        !get_real_argument( 0.0, &threshold ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -1, &bitlist_sizes[VIO_X] );
    (void) get_int_argument( -1, &bitlist_sizes[VIO_Y] );
    (void) get_int_argument( -1, &bitlist_sizes[VIO_Z] );
    (void) get_real_argument( 0.0, &tolerance );

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != VIO_OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        if( bitlist_sizes[dim] < 1 )
            bitlist_sizes[dim] = sizes[dim] + 1;
    }

    volume_transform = get_voxel_to_world_transform( volume );

    make_identity_transform( &new_to_old );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        Transform_elem( new_to_old, dim, dim ) = (VIO_Real) (sizes[dim]+1) /
                                                 (VIO_Real) bitlist_sizes[dim];
        Transform_elem( new_to_old, dim, 3 ) = 0.5 * (VIO_Real) (sizes[dim]+1) /
                                               (VIO_Real) bitlist_sizes[dim] - 1.0;
    }

    create_linear_transform( &mod_transform, &new_to_old );
    concat_general_transforms( &mod_transform, volume_transform,
                               &new_transform );
    delete_general_transform( &mod_transform );

    create_bitlist_3d( bitlist_sizes[0], bitlist_sizes[1], bitlist_sizes[2],
                       &bitlist );

    label_surface_voxels( volume, &bitlist, bitlist_sizes, &new_transform,
                          threshold, tolerance );

    transform_filename = concat_strings( output_prefix, ".xfm" );
    bitlist_filename = concat_strings( output_prefix, ".btl" );

    (void) output_transform_file( transform_filename, NULL, &new_transform );

    if( open_file( bitlist_filename, WRITE_FILE, BINARY_FORMAT, &file ) != VIO_OK )
        return( 1 );

    (void) io_int( file, WRITE_FILE, BINARY_FORMAT, &bitlist_sizes[VIO_X] );
    (void) io_int( file, WRITE_FILE, BINARY_FORMAT, &bitlist_sizes[VIO_Y] );
    (void) io_int( file, WRITE_FILE, BINARY_FORMAT, &bitlist_sizes[VIO_Z] );

    (void) io_bitlist_3d( file, WRITE_FILE, &bitlist );

    (void) close_file( file );
    
    return( 0 );
}

static  void  label_surface_voxels(
    VIO_Volume               volume,
    bitlist_3d_struct    *bitlist,
    int                  bitlist_sizes[],
    VIO_General_transform    *bitlist_to_world,
    VIO_Real                 threshold,
    VIO_Real                 tolerance )
{
    int                 x, y, z, dim, dx, dy, dz;
    VIO_Real                voxel[VIO_N_DIMENSIONS];
    VIO_Real                min_voxel[VIO_N_DIMENSIONS], max_voxel[VIO_N_DIMENSIONS];
    int                 sizes[VIO_N_DIMENSIONS];
    VIO_progress_struct     progress;
    VIO_General_transform   *volume_transform, inverse, v_to_v;

    volume_transform = get_voxel_to_world_transform( volume );
    create_inverse_general_transform( volume_transform, &inverse );

    concat_general_transforms( bitlist_to_world, &inverse, &v_to_v );

    get_volume_sizes( volume, sizes );

    initialize_progress_report( &progress, FALSE,
               bitlist_sizes[VIO_X] * bitlist_sizes[VIO_Y], "Finding surface voxels" );

    for_less( x, 0, bitlist_sizes[VIO_X] )
    for_less( y, 0, bitlist_sizes[VIO_Y] )
    {
        for_less( z, 0, bitlist_sizes[VIO_Z] )
        {
            for_less( dx, 0, 2 )
            for_less( dy, 0, 2 )
            for_less( dz, 0, 2 )
            {
                voxel[VIO_X] = (VIO_Real) x - 0.5 + (VIO_Real) dx;
                voxel[VIO_Y] = (VIO_Real) y - 0.5 + (VIO_Real) dy;
                voxel[VIO_Z] = (VIO_Real) z - 0.5 + (VIO_Real) dz;

                general_transform_point( &v_to_v,
                                         voxel[VIO_X], voxel[VIO_Y], voxel[VIO_Z],
                                         &voxel[VIO_X], &voxel[VIO_Y], &voxel[VIO_Z] );

                for_less( dim, 0, VIO_N_DIMENSIONS )
                {
                    if( dx == 0 && dy == 0 && dz == 0 ||
                        voxel[dim] < min_voxel[dim] )
                    {
                        min_voxel[dim] = voxel[dim];
                    }
                    if( dx == 0 && dy == 0 && dz == 0 ||
                        voxel[dim] > max_voxel[dim] )
                    {
                        max_voxel[dim] = voxel[dim];
                    }
                }
            }

            for_less( dim, 0, VIO_N_DIMENSIONS )
            {
                min_voxel[dim] -= tolerance;
                max_voxel[dim] += tolerance;
            }

            if( volume_range_contains_threshold( volume, min_voxel, max_voxel,
                                                 threshold ) )
            {
                set_bitlist_bit_3d( bitlist, x, y, z, TRUE );
            }
        }

        update_progress_report( &progress, y + 1 + x * bitlist_sizes[VIO_Y] );
    }

    terminate_progress_report( &progress );
}

static  VIO_BOOL  volume_range_contains_threshold(
    VIO_Volume   volume,
    VIO_Real     min_voxel[],
    VIO_Real     max_voxel[],
    VIO_Real     threshold )
{
    int      dim, sizes[VIO_N_DIMENSIONS];
    VIO_Real     value, x, y, z, voxel[VIO_N_DIMENSIONS];
    VIO_BOOL  x_is_int, y_is_int, z_is_int, above, below;

    get_volume_sizes( volume, sizes );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        if( min_voxel[dim] < -1.0 )
            min_voxel[dim] = -1.0;
        if( max_voxel[dim] > (VIO_Real) sizes[dim] )
            max_voxel[dim] = (VIO_Real) sizes[dim];
    }

    above = FALSE;
    below = FALSE;

    x = min_voxel[VIO_X];
    x_is_int = VIO_IS_INT(x) && x >= 0.0 && (int) x < sizes[0];
    while( TRUE )
    {
        y = min_voxel[VIO_Y];
        y_is_int = VIO_IS_INT(y) && y >= 0.0 && (int) y < sizes[1];
        while( TRUE )
        {
            z = min_voxel[VIO_Z];
            z_is_int = VIO_IS_INT(z) && z >= 0.0 && (int) z < sizes[2];
            while( TRUE )
            {
                if( x_is_int && y_is_int && z_is_int )
                {
                    value = get_volume_real_value( volume,
                                                   (int) x, (int) y, (int) z,
                                                   0, 0 );
                }
                else
                {
                    voxel[VIO_X] = x;
                    voxel[VIO_Y] = y;
                    voxel[VIO_Z] = z;
                    (void) evaluate_volume( volume, voxel, NULL, 0, FALSE,
                                            0.0, &value, NULL, NULL );
                }

                if( value == threshold )
                    return( TRUE );
                else if( value < threshold )
                {
                    if( above )
                        return( TRUE );
                    below = TRUE;
                }
                else
                {
                    if( below )
                        return( TRUE );
                    above = TRUE;
                }

                if( z >= max_voxel[VIO_Z] )
                    break;

                z += 1.0;
                if( !VIO_IS_INT(z) )
                    z = (VIO_Real) (int) z;
                if( z > max_voxel[VIO_Z] )
                {
                    z = max_voxel[VIO_Z];
                    z_is_int = VIO_IS_INT(z) && z >= 0.0 && (int) z < sizes[2];
                }
                else
                    z_is_int = FALSE;
            }

            if( y >= max_voxel[VIO_Y] )
                break;

            y += 1.0;
            if( !VIO_IS_INT(y) )
                y = (VIO_Real) (int) y;
            if( y > max_voxel[VIO_Y] )
            {
                y = max_voxel[VIO_Y];
                y_is_int = VIO_IS_INT(y);
                y_is_int = VIO_IS_INT(y) && y >= 0.0 && (int) y < sizes[1];
            }
            else
                y_is_int = FALSE;
        }

        if( x >= max_voxel[VIO_X] )
            break;

        x += 1.0;
        if( !VIO_IS_INT(x) )
            x = (VIO_Real) (int) x;
        if( x > max_voxel[VIO_X] )
        {
            x = max_voxel[VIO_X];
            x_is_int = VIO_IS_INT(x) && x >= 0.0 && (int) x < sizes[0];
        }
        else
            x_is_int = FALSE;
    }

    return( FALSE );
}
