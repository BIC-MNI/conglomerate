#include  <volume_io.h>
#include  <bicpl.h>

#define  X_SIZE   50
#define  Y_SIZE   50
#define  Z_SIZE   50

private  void  scan_segment_to_volume(
    VIO_Volume   volume,
    VIO_Real     x1,
    VIO_Real     y1,
    VIO_Real     x2,
    VIO_Real     y2 );

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Status               status;
    VIO_STR               output_filename;
    int                  i, x, y, z, sizes[N_DIMENSIONS];
    VIO_Real                 voxel[N_DIMENSIONS], world[N_DIMENSIONS];
    VIO_Real                 x_centre, y_centre, x2, y2, xw, yw, zw;
    VIO_Real                 angle, length;
    VIO_Real                 x_dir[N_DIMENSIONS];
    VIO_Real                 y_dir[N_DIMENSIONS];
    VIO_Real                 z_dir[N_DIMENSIONS];
    char                 history[10000];
    VIO_Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &x_centre ) ||
        !get_real_argument( 0.0, &y_centre ) )
    {
        print( "Need arguments.\n" );
        return( 1 );
    }

    volume = create_volume( 3, XYZ_dimension_names, NC_BYTE, FALSE,
                            0.0, 255.0 );

    sizes[X] = X_SIZE;
    sizes[Y] = Y_SIZE;
    sizes[Z] = Z_SIZE;
    set_volume_sizes( volume, sizes );

    set_volume_voxel_range( volume, 0.0, 255.0 );
    set_volume_real_range( volume, 0.0, (VIO_Real) X_SIZE );

/*
    x_dir[0] = 1.2;
    x_dir[1] = 0.0;
    x_dir[2] = 0.88;

    y_dir[0] = 1.1;
    y_dir[1] = 0.0;
    y_dir[2] = -1.3;

    z_dir[0] = 0.0;
    z_dir[1] = -1.0;
    z_dir[2] = 1.1;
    set_volume_direction_cosine( volume, X, x_dir );
    set_volume_direction_cosine( volume, Y, y_dir );
    set_volume_direction_cosine( volume, Z, z_dir );
    volume->spatial_axes[2] = -1;
    volume->spatial_axes[1] = -1;

    world[X] = 2.0 * 1.2;
    world[Y] = 0.0;
    world[Z] = 2.0 * 0.88;
*/

    voxel[X] = ((VIO_Real) sizes[X] - 1.0) / 2.0;
    voxel[Y] = ((VIO_Real) sizes[Y] - 1.0) / 2.0;
    voxel[Z] = ((VIO_Real) sizes[Z] - 1.0) / 2.0;

    world[X] = 0.0;
    world[Y] = 0.0;
    world[Z] = 0.0;

    set_volume_translation( volume, voxel, world );

    convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );

    print( "%g %g %g\n", xw, yw, zw );

    alloc_volume_data( volume );

    for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
            for_less( z, 0, sizes[Z] )
                set_volume_voxel_value( volume, x, y, z, 0, 0, 255.0 );

    while( get_real_argument( 0.0, &angle ) &&
           get_real_argument( 0.0, &length ) )
    {
        x2 = x_centre + length * cos( angle * DEG_TO_RAD );
        y2 = y_centre + length * sin( angle * DEG_TO_RAD );
        scan_segment_to_volume( volume, x_centre, y_centre, x2, y2 );
    }

    print( "Writing %s\n", output_filename );

    (void) strcpy( history, "Created by:  " );

    for_less( i, 0, argc )
    {
        (void) strcat( history, " " );
        (void) strcat( history, argv[i] );
    }

    status = output_volume( output_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            volume, history, (minc_output_options *) NULL );

    return( status != VIO_OK );
}

private  VIO_Real  get_distance_from_segment(
    VIO_Real     x,
    VIO_Real     y,
    VIO_Real     x1,
    VIO_Real     y1,
    VIO_Real     x2,
    VIO_Real     y2 )
{
    VIO_Real    dx, dy, dist, dist1, dist2;
    VIO_Real    dx12, dy12;
    VIO_Real    len_squared;

    dx12 = x2 - x1;
    dy12 = y2 - y1;

    len_squared = dx12 * dx12 + dy12 * dy12;
    if( len_squared != 0.0 )
    {
        dx = x - x1;
        dy = y - y1;

        dist = (dx * dx12 + dy * dy12) / len_squared;

        if( dist >= 0.0 && dist <= 1.0 )
        {
            dx = dx - dist * dx12;
            dy = dy - dist * dy12;
            return( sqrt( dx * dx + dy * dy ) );
        }
    }

    dist1 = (x - x1) * (x - x1) + (y - y1) * (y - y1);
    dist2 = (x - x2) * (x - x2) + (y - y2) * (y - y2);

    if( dist1 < dist2 )
        return( sqrt( dist1 ) );
    else
        return( sqrt( dist2 ) );
}

private  void  scan_segment_to_volume(
    VIO_Volume   volume,
    VIO_Real     x1,
    VIO_Real     y1,
    VIO_Real     x2,
    VIO_Real     y2 )
{
    int   x, y, z, sizes[N_DIMENSIONS];
    VIO_Real  x_w, y_w, z_w, dist, voxel, value, voxel_pos[N_DIMENSIONS];

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[X] )
    {
        voxel_pos[X] = (VIO_Real) x;
        for_less( y, 0, sizes[Y] )
        {
            voxel_pos[Y] = (VIO_Real) y;
            for_less( z, 0, sizes[Z] )
            {
                voxel_pos[Z] = (VIO_Real) z;
                convert_voxel_to_world( volume, voxel_pos, &x_w, &y_w, &z_w );

                dist = get_distance_from_segment( x_w, y_w, x1, y1, x2, y2 );

                value = get_volume_real_value( volume, x, y, z, 0, 0 );

                if( dist < value )
                {
                    voxel = convert_value_to_voxel( volume, dist );
                    if( voxel > 255.0 )
                        voxel = 255.0;

                    set_volume_voxel_value( volume, x, y, z, 0, 0, voxel );
                }
            }
        }
    }
}
