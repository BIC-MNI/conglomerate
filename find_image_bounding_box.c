#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>
#include  <bicpl/images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING         input_filename, bg_name;
    int            x, y, x_min, x_max, y_min, y_max;
    pixels_struct  pixels;
    Colour         bg_colour, col;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &bg_name ) )
    {
        print_error( "Usage: %s input.rgb bg_colour\n", argv[0] );
        return( 1 );
    }

    if( input_rgb_file( input_filename, &pixels ) != OK )
        return( 1 );

    if( pixels.pixel_type != RGB_PIXEL )
    {
        print( "Pixels must be RGB type.\n" );
        return( 1 );
    }

    
    bg_colour = convert_string_to_colour( bg_name );

    for_less( x_min, 0, pixels.x_size )
    {
        for_less( y, 0, pixels.y_size )
        {
            col = PIXEL_RGB_COLOUR(pixels,x_min,y);
            if( col != bg_colour )
                break;
        }
        if( y < pixels.y_size )
            break;
    }

    for_down( x_max, pixels.x_size-1, 0 )
    {
        for_less( y, 0, pixels.y_size )
        {
            col = PIXEL_RGB_COLOUR(pixels,x_max,y);
            if( col != bg_colour )
                break;
        }
        if( y < pixels.y_size )
            break;
    }

    for_less( y_min, 0, pixels.y_size )
    {
        for_less( x, 0, pixels.x_size )
        {
            col = PIXEL_RGB_COLOUR(pixels,x,y_min);
            if( col != bg_colour )
                break;
        }
        if( x < pixels.x_size )
            break;
    }

    for_down( y_max, pixels.y_size-1, 0 )
    {
        for_less( x, 0, pixels.x_size )
        {
            col = PIXEL_RGB_COLOUR(pixels,x,y_max);
            if( col != bg_colour )
                break;
        }
        if( x < pixels.x_size )
            break;
    }

    print( "Bounding box: %d %d %d %d\n", x_min, x_max, y_min, y_max );
    return( 0 );
}
