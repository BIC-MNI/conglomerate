#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    Volume             volume;
    STRING             input_filename, output_filename;
    int                x, y;
    int                sizes[MAX_DIMENSIONS];
    STRING             dim_names4d[] = {
                                         MIzspace,
                                         MIxspace,
                                         MIyspace,
                                         MIvector_dimension };
    Real               rgba[4], scale_factor;
    minc_input_options options;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_real_argument( 0.0, &scale_factor ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print( "Usage: %s input.mnc  scale  output.mnc\n", argv[0] );
        return( 1 );
    }

    set_default_minc_input_options( &options );
    set_minc_input_vector_to_scalar_flag( &options, FALSE );
    set_minc_input_vector_to_colour_flag( &options, FALSE );

    if( input_volume( input_filename, 4, dim_names4d,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, &options ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    rgba[3] = 1.0;

    for_less( x, 0, sizes[1] )
    {
        for_less( y, 0, sizes[2] )
        {
            get_volume_value_hyperslab( volume, 0, x, y, 0, 0,
                                                 1, 1, 1, sizes[3], 0, rgba );

            rgba[0] *= scale_factor;
            rgba[1] *= scale_factor;
            rgba[2] *= scale_factor;

            set_volume_value_hyperslab( volume, 0, x, y, 0, 0,
                                                1, 1, 1, sizes[3], 0, rgba );
        }
    }

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, volume, "Scaled\n", NULL );

    return( 0 );
}
