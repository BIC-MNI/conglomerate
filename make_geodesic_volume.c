
#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    char   executable[] )
{
    STRING   usage_str = "\n\
Usage: %s   input.mnc  output.mnc   threshold  x y z \n\
            [x_size]  [y_size]  [z_size]\n\
\n\
     Creates the geodesic volume with respect to the (x, y, z) points.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING              input_filename, output_filename, *dim_names;
    Volume              volume, geodesic;
    int                 dim;
    int                 sizes[N_DIMENSIONS], geo_sizes[N_DIMENSIONS];
    Real                threshold, x_world, y_world, z_world;
    Transform           new_to_old;
    General_transform   *volume_transform, mod_transform, new_transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &threshold ) ||
        !get_real_argument( 0.0, &x_world ) ||
        !get_real_argument( 0.0, &y_world ) ||
        !get_real_argument( 0.0, &z_world ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -1, &geo_sizes[X] );
    (void) get_int_argument( -1, &geo_sizes[Y] );
    (void) get_int_argument( -1, &geo_sizes[Z] );

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    dim_names = get_volume_dimension_names( volume );

    geodesic = create_volume( N_DIMENSIONS, dim_names, NC_BYTE, FALSE,
                              0.0, 0.0 );

    delete_dimension_names( volume, dim_names );

    get_volume_sizes( volume, sizes );

    for_less( dim, 0, N_DIMENSIONS )
    {
        if( geo_sizes[dim] < 1 )
            geo_sizes[dim] = sizes[dim];
    }

    set_volume_sizes( geodesic, geo_sizes );
    alloc_volume_data( geodesic );

    volume_transform = get_voxel_to_world_transform( volume );

    make_identity_transform( &new_to_old );

    for_less( dim, 0, N_DIMENSIONS )
    {
        Transform_elem( new_to_old, dim, dim ) = (Real) geo_sizes[dim] /
                                                 (Real) sizes[dim];
        Transform_elem( new_to_old, dim, 3 ) = 0.5 * (Real) geo_sizes[dim] /
                                               (Real) sizes[dim] - 0.5;
    }

    create_linear_transform( &mod_transform, &new_to_old );
    concat_general_transforms( &mod_transform, volume_transform,
                               &new_transform );
    set_voxel_to_world_transform( geodesic, &new_transform );
    delete_general_transform( &mod_transform );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, geodesic, "Geodesic volume\n", NULL );
    
    return( 0 );
}
