#include  <internal_volume_io.h>
#include  <bicpl.h>

private int  count_voxels(
    Volume       volume1,
    Volume       volume2,
    Transform    *v1_to_v2,
    Transform    *v2_to_v1,
    Real         voxel[] );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input1.mnc  input2.mnc  input.xfm\n\
\n\
\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    General_transform    *trans1, *trans2;
    Transform            v1_to_v2, *voxel_to_world1, *voxel_to_world2;
    Transform            transform, v2_to_v1, world1_to_world2;
    Transform            world_to_voxel2;
    STRING               volume1_filename, volume2_filename;
    STRING               transform_filename;
    int                  sizes1[MAX_DIMENSIONS];
    int                  i, dim, n_points, n_voxels;
    Volume               volume1, volume2;
    Real                 avg, voxel_size1, voxel_size2;
    Real                 voxel[MAX_DIMENSIONS];
    Real                 separations1[MAX_DIMENSIONS];
    Real                 separations2[MAX_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume1_filename ) ||
        !get_string_argument( "", &volume2_filename ) ||
        !get_string_argument( "", &transform_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume_header_only( volume1_filename, 3, NULL, &volume1, NULL
                                              ) != OK )
        return( 1 );

    if( input_volume_header_only( volume2_filename, 3, NULL, &volume2, NULL
                                              ) != OK )
        return( 1 );

    if( read_transform_file( transform_filename, &transform ) != OK )
        return( 1 );

    (void) get_int_argument( 100, &n_points );

    get_volume_sizes( volume1, sizes1 );

    trans1 = get_voxel_to_world_transform( volume1 );
    trans2 = get_voxel_to_world_transform( volume2 );

    if( get_transform_type(trans1) != LINEAR ||
        get_transform_type(trans2) != LINEAR )
    {
        print( "Error in volume transform types.\n" );
        return( 1 );
    }

    voxel_to_world1 = get_linear_transform_ptr( trans1 );
    voxel_to_world2 = get_linear_transform_ptr( trans2 );

    compute_transform_inverse( &transform, &world1_to_world2 );
    compute_transform_inverse( voxel_to_world2, &world_to_voxel2 );

    concat_transforms( &v1_to_v2, voxel_to_world1, &world1_to_world2 );
    concat_transforms( &v1_to_v2, &v1_to_v2, &world_to_voxel2 );

    compute_transform_inverse( &v1_to_v2, &v2_to_v1 );

    n_voxels = 0;

    for_less( i, 0, n_points )
    {
        for_less( dim, 0, N_DIMENSIONS )
            voxel[dim] = get_random_0_to_1() * sizes1[dim];

        n_voxels += count_voxels( volume1, volume2, &v1_to_v2, &v2_to_v1,
                                  voxel );
    }

    get_volume_separations( volume1, separations1 );
    get_volume_separations( volume2, separations2 );


    voxel_size1 = separations1[X] * separations1[Y] * separations1[Z];
    voxel_size2 = separations2[X] * separations2[Y] * separations2[Z];

    avg = (Real) n_voxels / (Real) n_points * voxel_size2 / voxel_size1;


    print( "Ratio: %g\n", avg );

    return( 0 );
}

private int  count_voxels(
    Volume       volume1,
    Volume       volume2,
    Transform    *v1_to_v2,
    Transform    *v2_to_v1,
    Real         voxel[] )
{
    int   dx, dy, dz, dim, n_voxels, x, y, z;
    int   x_start, x_end, y_start, y_end, z_start, z_end;
    Real  x1, y1, z1;
    Real  v2[N_DIMENSIONS];
    Real  min_voxel[N_DIMENSIONS];
    Real  max_voxel[N_DIMENSIONS];

    for_less( dx, 0, 2 )
    for_less( dy, 0, 2 )
    for_less( dz, 0, 2 )
    {
        transform_point( v1_to_v2, voxel[X] - 0.5 + (Real) dx,
                                   voxel[Y] - 0.5 + (Real) dy,
                                   voxel[Z] - 0.5 + (Real) dz,
                         &v2[X], &v2[Y], &v2[Z] );

        if( dx == 0 && dy == 0 && dz == 0 )
        {
            for_less( dim, 0, N_DIMENSIONS )
            {
                min_voxel[dim] = v2[dim];
                max_voxel[dim] = v2[dim];
            }
        }
        else
        {
            for_less( dim, 0, N_DIMENSIONS )
            {
                if( v2[dim] < min_voxel[dim] )
                    min_voxel[dim] = v2[dim];
                else if( v2[dim] > max_voxel[dim] )
                    max_voxel[dim] = v2[dim];
            }
        }
    }

    x_start = CEILING( min_voxel[X] );
    y_start = CEILING( min_voxel[Y] );
    z_start = CEILING( min_voxel[Z] );
    x_end = FLOOR( max_voxel[X] );
    y_end = FLOOR( max_voxel[Y] );
    z_end = FLOOR( max_voxel[Z] );

    n_voxels = 0;

    for_inclusive( x, x_start, x_end )
    for_inclusive( y, y_start, y_end )
    for_inclusive( z, z_start, z_end )
    {
        transform_point( v2_to_v1, (Real) x, (Real) y, (Real) z,
                         &x1, &y1, &z1 );

        if( x1 >= voxel[X] - 0.5 && x1 <= voxel[X] + 0.5 &&
            y1 >= voxel[Y] - 0.5 && y1 <= voxel[Y] + 0.5 &&
            z1 >= voxel[Z] - 0.5 && z1 <= voxel[Z] + 0.5 )
        {
            ++n_voxels;
        }
    }

    return( n_voxels );
}
