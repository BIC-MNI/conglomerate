#include  <internal_volume_io.h>
#include  <images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING            input_filename, output_filename1, output_filename2;
    int               x, y, width;
    pixels_struct     in_pixels, out_pixels;
    Colour            col;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_int_argument( 0, &width ) ||
        !get_string_argument( "", &output_filename1 ) ||
        !get_string_argument( "", &output_filename2 ) )
    {
        print_error( "Usage: %s input.rgb width output1.rgb output2.rgb\n",
                     argv[0] );
        return( 1 );
    }

    if( input_rgb_file( input_filename, &in_pixels ) != OK )
        return( 1 );

    if( in_pixels.pixel_type != RGB_PIXEL || in_pixels.x_size < width )
    {
        print_error( "Pixels are not the same height or not RGB.\n" );
        return( 0 );
    }

    initialize_pixels( &out_pixels, 0, 0, width, in_pixels.y_size,
                       1.0, 1.0, RGB_PIXEL );

    for_less( x, 0, width )
    {
        for_less( y, 0, in_pixels.y_size )
        {
            col = PIXEL_RGB_COLOUR(in_pixels,x,y);
            PIXEL_RGB_COLOUR(out_pixels,x,y) = col;
        }
    }

    if( output_rgb_file( output_filename1, &out_pixels ) != OK )
        return( 1 );

    delete_pixels( &out_pixels );

    initialize_pixels( &out_pixels, 0, 0,
                       in_pixels.x_size - width, in_pixels.y_size,
                       1.0, 1.0, RGB_PIXEL );

    for_less( x, 0, in_pixels.x_size - width )
    {
        for_less( y, 0, in_pixels.y_size )
        {
            col = PIXEL_RGB_COLOUR(in_pixels,x+width,y);
            PIXEL_RGB_COLOUR(out_pixels,x,y) = col;
        }
    }

    if( output_rgb_file( output_filename2, &out_pixels ) != OK )
        return( 1 );

    delete_pixels( &out_pixels );

    return( 0 );
}
