#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING            input_filename, output_filename;
    int               x, y;
    pixels_struct     pixels;
    Colour            col;
    Real              r, g, b, a;
    Real              r_scale, g_scale, b_scale, a_scale;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &r_scale ) )
    {
        print( "Usage: %s input.rgb output.rgb  strength\n", argv[0] );
        return( 1 );
    }

    if( !get_real_argument( 0.0, &g_scale ) ||
        !get_real_argument( 0.0, &b_scale ) ||
        !get_real_argument( 0.0, &a_scale ) )
    {
        g_scale = r_scale;
        b_scale = r_scale;
        a_scale = 1.0;
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
            r = r_scale * get_Colour_r_0_1( col );
            g = g_scale * get_Colour_g_0_1( col );
            b = b_scale * get_Colour_b_0_1( col );
            a = a_scale * get_Colour_a_0_1( col );

            if( r > 1.0 ) r = 1.0;
            if( g > 1.0 ) g = 1.0;
            if( b > 1.0 ) b = 1.0;
            if( a > 1.0 ) a = 1.0;
            col = make_rgba_Colour_0_1( r, g, b, a );
            PIXEL_RGB_COLOUR(pixels,x,y) = col;
        }
    }

    if( output_rgb_file( output_filename, &pixels ) != OK )
        return( 1 );

    return( 0 );
}
