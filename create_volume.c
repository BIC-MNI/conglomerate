#include  <mni.h>

#define  X_SIZE   50
#define  Y_SIZE   50
#define  Z_SIZE   50

private  void  scan_segment_to_volume(
    Volume   volume,
    Real     x1,
    Real     y1,
    Real     x2,
    Real     y2 );

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *output_filename;
    int                  i, x, y, z, sizes[N_DIMENSIONS];
    Real                 voxel[N_DIMENSIONS], world[N_DIMENSIONS];
    Real                 x_centre, y_centre, x2, y2;
    Real                 angle, length;
    char                 history[10000];
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) ||
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
    set_volume_real_range( volume, 0.0, (Real) X_SIZE );

    voxel[X] = (sizes[X] - 1.0) / 2.0;
    voxel[Y] = (sizes[Y] - 1.0) / 2.0;
    voxel[Z] = (sizes[Z] - 1.0) / 2.0;
    world[X] = 0.0;
    world[Y] = 0.0;
    world[Z] = 0.0;

    set_volume_translation( volume, voxel, world );

    alloc_volume_data( volume );

    for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
            for_less( z, 0, sizes[Z] )
                SET_VOXEL_3D( volume, x, y, z, 255.0 );

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

    return( status != OK );
}

private  Real  get_distance_from_segment(
    Real     x,
    Real     y,
    Real     x1,
    Real     y1,
    Real     x2,
    Real     y2 )
{
    Real    dx, dy, dist, dist1, dist2;
    Real    dx12, dy12;
    Real    len_squared;

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
    Volume   volume,
    Real     x1,
    Real     y1,
    Real     x2,
    Real     y2 )
{
    int   x, y, z, sizes[N_DIMENSIONS];
    Real  x_w, y_w, z_w, dist, voxel, value, voxel_pos[N_DIMENSIONS];

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[X] )
    {
        voxel_pos[X] = (Real) x;
        for_less( y, 0, sizes[Y] )
        {
            voxel_pos[Y] = (Real) y;
            for_less( z, 0, sizes[Z] )
            {
                voxel_pos[Z] = (Real) z;
                convert_voxel_to_world( volume, voxel_pos, &x_w, &y_w, &z_w );

                dist = get_distance_from_segment( x_w, y_w, x1, y1, x2, y2 );

                GET_VALUE_3D( value, volume, x, y, z );

                if( dist < value )
                {
                    voxel = CONVERT_VALUE_TO_VOXEL( volume, dist );
                    if( voxel > 255.0 )
                        voxel = 255.0;

                    SET_VOXEL_3D( volume, x, y, z, voxel );
                }
            }
        }
    }
}
