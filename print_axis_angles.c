#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.mnc  to_tal.xfm\n\
\n\
     Displays the angles in degrees between the native space and talairach\n\
     axes in talairach space.\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename, transform_filename;
    General_transform    transform, *volume_transform;
    Real                 c, angle, ox, oy, oz, vx, vy, vz;
    Vector               axis[N_DIMENSIONS], axis_in_tal;
    Volume               volume;
    Transform            *t;
    int                  dim;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &transform_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume_header_only( volume_filename, 3, NULL, &volume, NULL) !=OK)
        return( 1 );

    if( input_transform_file( transform_filename, &transform ) )
        return( 1 );

    volume_transform = get_voxel_to_world_transform( volume );

    if( get_transform_type(volume_transform) != LINEAR )
    {
        print( "Volume transform must be linear.\n" );
        return( 1 );
    }

    t = get_linear_transform_ptr( volume_transform );

    get_transform_x_axis( t, &axis[0] );
    get_transform_y_axis( t, &axis[1] );
    get_transform_z_axis( t, &axis[2] );

    general_transform_point( &transform, 0.0, 0.0, 0.0, &ox, &oy, &oz );

    for_less( dim, 0, N_DIMENSIONS )
    {
        general_transform_point( &transform,
                                 RVector_x(axis[dim]),
                                 RVector_y(axis[dim]),
                                 RVector_z(axis[dim]),
                                 &vx, &vy, &vz );
        fill_Vector( axis_in_tal, vx - ox, vy - oy, vz - oz );
        NORMALIZE_VECTOR( axis_in_tal, axis_in_tal );
        c = RVector_coord( axis_in_tal, dim );
        c = FABS( c );
        angle = RAD_TO_DEG * acos( c );
        print( "%c axis: %.4g degrees\n", "XYZ"[dim], angle );
    }

    return( 0 );
}
