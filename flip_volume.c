#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    char  executable[] )
{
    char  usage_str[] = "\n\
Usage: %s input.mnc output.mnc\n\
\n\
     Interchanges voxel values with their left-right opposites, thereby\n\
     flipping the volume.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    Volume               volume;
    Real                 voxel_left, voxel_right;
    int                  x, y, z, sizes[N_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[0]/2 )
    for_less( y, 0, sizes[1] )
    for_less( z, 0, sizes[2] )
    {
        GET_VOXEL_3D( voxel_left, volume, x, y, z );
        GET_VOXEL_3D( voxel_right, volume, sizes[0] - 1 - x, y, z );
        SET_VOXEL_3D( volume, x, y, z, voxel_right );
        SET_VOXEL_3D( volume, sizes[0] - 1 - x, y, z, voxel_left );
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, volume, input_filename,
                                   "flip_volume", (minc_output_options *) NULL);

    delete_volume( volume );

    return( 0 );
}