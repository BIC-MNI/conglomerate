
#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR     input_filename;
    VIO_Real       x, y, z, value;
    VIO_Volume     volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        return( 1 );
    }
 
    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    while( get_real_argument( 0.0, &x ) &&
           get_real_argument( 0.0, &y ) &&
           get_real_argument( 0.0, &z ) )
    {
        evaluate_volume_in_world( volume, x, y, z, -1, FALSE, 0.0,
                                  &value,
                                  NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL, NULL );

        print( "%g\n", value );
    }

    return( 0 );
}
