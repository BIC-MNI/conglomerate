#include  <mni.h>

int  main(
    int   argc,
    char  *argv[] )
{
    int                  x, y, z, sizes[N_DIMENSIONS];
    Real                 value;
    char                 *input_volume_filename;
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) )
    {
        print( "Usage: %s  volume_file\n", argv[0] );
        return( 1 );
    }

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VALUE_3D( value, volume, x, y, z );
                printf( "%g\n", value );
            }
        }
    }

    return( 0 );
}
