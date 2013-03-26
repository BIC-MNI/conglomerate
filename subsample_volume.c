#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Volume     volume, new_volume;
    VIO_Status     status;
    int        nx, ny, nz;
    VIO_STR     input_filename, output_filename, history;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s input.mnc output.mnc nx ny nz\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 50, &nx );
    (void) get_int_argument( 50, &ny );
    (void) get_int_argument( 50, &nz );

    status = input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) ;

    if( status != OK )
        return( 1 );

    new_volume = smooth_resample_volume( volume, nx, ny, nz );

    history = "resampled";

    status = output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                            0.0, 0.0,
                            new_volume, history, (minc_output_options *) NULL );

    return( 0 );
}
