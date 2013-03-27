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
    int                  i;
    int                  x, y, z, x_size, y_size, z_size, sizes[N_DIMENSIONS];
    VIO_Real                 voxel[N_DIMENSIONS], world[N_DIMENSIONS];
    VIO_Real                 xw, yw, zw, dx, dy, dz, max_dist;
    char                 history[10000];
    VIO_Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) )
    {
        print( "Need arguments.\n" );
        return( 1 );
    }

    (void) get_int_argument( X_SIZE, &x_size );
    (void) get_int_argument( Y_SIZE, &y_size );
    (void) get_int_argument( Z_SIZE, &z_size );

    volume = create_volume( 3, XYZ_dimension_names, NC_BYTE, FALSE,
                            0.0, 255.0 );

    sizes[X] = x_size;
    sizes[Y] = y_size;
    sizes[Z] = z_size;
    set_volume_sizes( volume, sizes );

    dx = (VIO_Real) x_size / 2.0;
    dy = (VIO_Real) y_size / 2.0;
    dz = (VIO_Real) z_size / 2.0;

    max_dist = dx * dx + dy * dy + dz * dz;

    set_volume_voxel_range( volume, 0.0, 255.0 );
    set_volume_real_range( volume, 0.0, max_dist );

    voxel[X] = ((VIO_Real) sizes[X] - 1.0) / 2.0;
    voxel[Y] = ((VIO_Real) sizes[Y] - 1.0) / 2.0;
    voxel[Z] = ((VIO_Real) sizes[Z] - 1.0) / 2.0;

    world[X] = 0.0;
    world[Y] = 0.0;
    world[Z] = 0.0;

    set_volume_translation( volume, voxel, world );

    alloc_volume_data( volume );

    for_less( x, 0, sizes[X] )
    {
        voxel[X] = (VIO_Real) x;
        voxel[Y] = 0.0;
        voxel[Z] = 0.0;
        convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
        dx = xw;

        for_less( y, 0, sizes[Y] )
        {
            voxel[X] = 0.0;
            voxel[Y] = (VIO_Real) y;
            voxel[Z] = 0.0;
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            dy = yw;
            for_less( z, 0, sizes[Z] )
            {
                voxel[X] = 0.0;
                voxel[Y] = 0.0;
                voxel[Z] = (VIO_Real) z;
                convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
                dz = zw;
                set_volume_real_value( volume, x, y, z, 0, 0,
                                       dx * dx + dy * dy + dz * dz );
            }
        }
    }

    print( "Writing %s\n", output_filename );

    (void) strcpy( history, "Created by:  " );

    for_less( i, 0, argc )
    {
        (void) strcat( history, " " );
        (void) strcat( history, argv[i] );
    }

    status = output_volume( output_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            volume, history, NULL );

    return( status != VIO_OK );
}
