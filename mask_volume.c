#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *mask_volume_filename;
    char                 *output_filename;
    Real                 set_value, mask_value;
    int                  x, y, z, sizes[MAX_DIMENSIONS];
    Volume               volume, mask_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &mask_volume_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  volume_file  mask_volume  output_volume\n",
               argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &set_value );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_volume( mask_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &mask_volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VALUE_3D( mask_value, mask_volume, x, y, z );
                if( mask_value == 0.0 )
                    SET_VOXEL_3D( volume, x, y, z, set_value );
            }
        }
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, volume_filename,
                                   "Masked", NULL );

    return( 0 );
}
