#include  <volume_io.h>
#include  <bicpl/images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING         input_filename, output_filename;
    Real           r_shade, g_shade, b_shade;
    Real           r, g, b;
    int            x, y, intensity;
    pixels_struct  pixels;
    Colour         dest, col;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0, &r_shade ) ||
        !get_real_argument( 0, &g_shade ) ||
        !get_real_argument( 0, &b_shade ) )
    {
        print( "Usage: %s input.rgb output.rgb  r_src_min g_src_min b_src_min\n", argv[0] );
        print( "               r_src_max g_src_max b_src_min\n" );
        print( "               r_dest b_dest g_dest\n" );
        return( 1 );
    }

    if( input_rgb_file( input_filename, &pixels ) != OK )
        return( 1 );

    if( pixels.pixel_type != RGB_PIXEL )
    {
        print( "Pixels must be RGB type.\n" );
        return( 1 );
    }

    for_less( x, 0, pixels.x_size )
    {
        for_less( y, 0, pixels.y_size )
        {
            col = PIXEL_RGB_COLOUR(pixels,x,y);
            intensity = get_Colour_luminance( col );

            if( intensity > 220 )
                PIXEL_RGB_COLOUR(pixels,x,y) = BLUE;
            else
                PIXEL_RGB_COLOUR(pixels,x,y) = col;
        }
    }

    if( output_rgb_file( output_filename, &pixels ) != OK )
        return( 1 );

    return( 0 );
}
