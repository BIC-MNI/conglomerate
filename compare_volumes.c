#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    int                  x, y, z;
    Real                 value1, value2;
    int                  sizes1[MAX_DIMENSIONS], sizes2[MAX_DIMENSIONS];
    char                 *input_volume1, *input_volume2;
    Volume               volume1, volume2;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume1 ) ||
        !get_string_argument( "", &input_volume2 ) )
    {
        print( "Usage: %s  volume_file1  volume_file2\n", argv[0] );
        return( 1 );
    }

    if( input_volume( input_volume1, 3, XYZ_dimension_names,
                      NC_BYTE, FALSE, 0.0, 0.0,
                      TRUE, &volume1, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_volume( input_volume2, 3, XYZ_dimension_names,
                      NC_BYTE, FALSE, 0.0, 0.0,
                      TRUE, &volume2, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume1, sizes1 );
    get_volume_sizes( volume2, sizes2 );

    if( sizes1[X] != sizes2[X] || sizes1[Y] != sizes2[Y] ||
        sizes1[Z] != sizes2[Z] )
    {
        print( "Sizes differ.\n" );
        return( 1 );
    }

    for_less( x, 0, sizes1[X] )
    {
        for_less( y, 0, sizes1[Y] )
        {
            for_less( z, 0, sizes1[Z] )
            {
                GET_VALUE_3D( value1, volume1, x, y, z );
                GET_VALUE_3D( value2, volume2, x, y, z );
                if( value1 != value2 )
                    print( "Volumes differ: %d %d %d:  %g %g.\n", x, y, z,
                           value1, value2 );
            }
        }
    }

    return( 0 );
}
