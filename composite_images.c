#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING            input_filename, output_filename;
    int               x, y;
    pixels_struct     pixels, top_pixels;
    Colour            bottom, top;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &input_filename ) )
    {
        print( "Usage: %s output.rgb input1.rgb  [input2.rgb] ...\n", argv[0] );
        return( 1 );
    }

    if( input_rgb_file( input_filename, &pixels ) != OK )
        return( 1 );

    if( pixels.pixel_type != RGB_PIXEL )
    {
        print( "Pixels must be RGB type.\n" );
        return( 1 );
    }

    while( get_string_argument( NULL, &input_filename ) )
    {
        if( input_rgb_file( input_filename, &top_pixels ) != OK )
            return( 1 );

        if( top_pixels.pixel_type != RGB_PIXEL )
        {
            print( "Pixels must be RGB type.\n" );
            return( 1 );
        }

        for_less( x, 0, pixels.x_size )
        {
            for_less( y, 0, pixels.y_size )
            {
                bottom = PIXEL_RGB_COLOUR(pixels,x,y);
                top = PIXEL_RGB_COLOUR(top_pixels,x,y);
                COMPOSITE_COLOURS( PIXEL_RGB_COLOUR(pixels,x,y), top, bottom );
            }
        }

        delete_pixels( &top_pixels );
    }

    if( output_rgb_file( output_filename, &pixels ) != OK )
        return( 1 );

    return( 0 );
}
