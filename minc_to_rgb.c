#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    Volume             volume;
    STRING             input_filename, output_filename;
    int                v[MAX_DIMENSIONS];
    int                n_dims, sizes[MAX_DIMENSIONS];
    Colour             colour;
    int                x, y;
    pixels_struct      pixels;
    minc_input_options options;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print( "Usage: %s input.mnc  output.rgb\n", argv[0] );
        return( 1 );
    }

    set_default_minc_input_options( &options );
    set_minc_input_vector_to_colour_flag( &options, TRUE );

    if( input_volume( input_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, &options ) != OK )
        return( 1 );

    if( get_volume_data_type(volume) != UNSIGNED_LONG )
    {
        print_error( "Volume is not an RGB Minc file.\n" );
        return( 1 );
    }

    n_dims = get_volume_n_dimensions( volume );
    get_volume_sizes( volume, sizes );

    initialize_pixels( &pixels, 0, 0, sizes[n_dims-2], sizes[n_dims-1],
                        1.0, 1.0, RGB_PIXEL );

    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
    v[3] = 0;
    v[4] = 0;

    for_less( x, 0, sizes[n_dims-2] )
    {
        v[n_dims-2] = x;
        for_less( y, 0, sizes[n_dims-1] )
        {
            v[n_dims-1] = x;
            colour = (Colour) get_volume_real_value( volume,
                            v[0], v[1], v[2], v[3], v[4] );

            PIXEL_RGB_COLOUR(pixels,x,y) = colour;
        }
    }

    (void) output_rgb_file( output_filename, &pixels );

    return( 0 );
}
