#include  <internal_volume_io.h>
#include  <images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING            input_filename1, input_filename2, output_filename;
    int               x, y;
    pixels_struct     in_pixels1, in_pixels2, out_pixels;
    Colour            col;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename1 ) ||
        !get_string_argument( "", &input_filename2 ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print_error( "Usage: %s input1.rgb input2.rgb output.rgb\n", argv[0] );
        return( 1 );
    }

    if( input_rgb_file( input_filename1, &in_pixels1 ) != OK )
        return( 1 );

    if( input_rgb_file( input_filename2, &in_pixels2 ) != OK )
        return( 1 );

    if( in_pixels1.pixel_type != RGB_PIXEL ||
        in_pixels2.pixel_type != RGB_PIXEL ||
        in_pixels1.y_size != in_pixels2.y_size )
    {
        print_error( "Pixels are not the same height or not RGB.\n" );
        return( 0 );
    }

    initialize_pixels( &out_pixels, 0, 0,
                       in_pixels1.x_size + in_pixels2.x_size, in_pixels1.y_size,
                       1.0, 1.0, RGB_PIXEL );

    for_less( x, 0, in_pixels1.x_size )
    {
        for_less( y, 0, in_pixels1.y_size )
        {
            col = PIXEL_RGB_COLOUR(in_pixels1,x,y);
            PIXEL_RGB_COLOUR(out_pixels,x,y) = col;
        }
    }

    for_less( x, 0, in_pixels2.x_size )
    {
        for_less( y, 0, in_pixels2.y_size )
        {
            col = PIXEL_RGB_COLOUR(in_pixels2,x,y);
            PIXEL_RGB_COLOUR(out_pixels,x+in_pixels1.x_size,y) = col;
        }
    }

    if( output_rgb_file( output_filename, &out_pixels ) != OK )
        return( 1 );

    return( 0 );
}
