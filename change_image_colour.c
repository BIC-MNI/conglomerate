#include  <volume_io.h>
#include  <bicpl.h>
#include  <bicpl/images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR         input_filename, output_filename;
    int            r_src_min, g_src_min, b_src_min, a_src_min;
    int            r_src_max, g_src_max, b_src_max, a_src_max;
    int            r_dest, g_dest, b_dest, a_dest;
    int            r, g, b, a;
    int            x, y;
    pixels_struct  pixels;
    Colour         dest, col;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_int_argument( 0, &r_src_min ) ||
        !get_int_argument( 0, &g_src_min ) ||
        !get_int_argument( 0, &b_src_min ) ||
        !get_int_argument( 0, &a_src_min ) ||
        !get_int_argument( 0, &r_src_max ) ||
        !get_int_argument( 0, &g_src_max ) ||
        !get_int_argument( 0, &b_src_max ) ||
        !get_int_argument( 0, &a_src_max ) ||
        !get_int_argument( 0, &r_dest ) ||
        !get_int_argument( 0, &g_dest ) ||
        !get_int_argument( 0, &b_dest ) ||
        !get_int_argument( 0, &a_dest ) )
    {
        print( "Usage: %s input.rgb output.rgb  r_src_min g_src_min b_src_min\n", argv[0] );
        print( "               r_src_max g_src_max b_src_min\n" );
        print( "               r_dest b_dest g_dest\n" );
        return( 1 );
    }

    if( input_rgb_file( input_filename, &pixels ) != VIO_OK )
        return( 1 );

    if( pixels.pixel_type != RGB_PIXEL )
    {
        print( "Pixels must be RGB type.\n" );
        return( 1 );
    }

    dest = make_rgba_Colour( r_dest, g_dest, b_dest, a_dest );

    for_less( x, 0, pixels.x_size )
    {
        for_less( y, 0, pixels.y_size )
        {
            col = PIXEL_RGB_COLOUR(pixels,x,y);
            r = get_Colour_r( col );
            g = get_Colour_g( col );
            b = get_Colour_b( col );
            a = get_Colour_a( col );
            if( r_src_min <= r && r <= r_src_max &&
                g_src_min <= g && g <= g_src_max &&
                b_src_min <= b && b <= b_src_max &&
                a_src_min <= a && a <= a_src_max )
                PIXEL_RGB_COLOUR(pixels,x,y) = dest;
        }
    }

    if( output_rgb_file( output_filename, &pixels ) != VIO_OK )
        return( 1 );

    return( 0 );
}
