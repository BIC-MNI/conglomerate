#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING            input_filename, output_filename;
    int               x, y, n_files, r, g, b, a;
    int               x_offset, y_offset, x_min, x_max, y_min, y_max;
    pixels_struct     pixels, top_pixels;
    Colour            bottom, top, col;
    BOOLEAN           add_flag;

    initialize_argument_processing( argc, argv );

    if( argc <= 1 )
    {
        print( "Usage: %s output.rgb [-add] input1.rgb  -xy [input2.rgb] ...\n", argv[0] );
        return( 1 );
    }

    output_filename = NULL;

    n_files = 0;
    add_flag = FALSE;
    x_offset = 0;
    y_offset = 0;

    while( get_string_argument( NULL, &input_filename ) )
    {
        if( equal_strings( input_filename, "-add" ) )
        {
            add_flag = TRUE;
            continue;
        }
        if( equal_strings( input_filename, "-composite" ) )
        {
            add_flag = FALSE;
            continue;
        }
        if( equal_strings( input_filename, "-xy" ) )
        {
            if( !get_int_argument( 0, &x_offset ) ||
                !get_int_argument( 0, &y_offset ) )
            {
                print_error( "Error in -xy arguments\n" );
                return( 1 );
            }
            continue;
        }
        if( output_filename == NULL )
        {
            output_filename = input_filename;
            continue;
        }

        if( input_rgb_file( input_filename, &top_pixels ) != OK )
            return( 1 );

        if( top_pixels.pixel_type != RGB_PIXEL )
        {
            print( "Pixels must be RGB type.\n" );
            return( 1 );
        }

        if( n_files == 0 )
            pixels = top_pixels;
        else
        {
            x_min = MAX( 0, x_offset );
            x_max = MIN( pixels.x_size-1, x_offset + top_pixels.x_size - 1 );
            y_min = MAX( 0, y_offset );
            y_max = MIN( pixels.y_size-1, y_offset + top_pixels.y_size - 1 );

            for_inclusive( x, x_min, x_max )
            {
                for_inclusive( y, y_min, y_max )
                {
                    bottom = PIXEL_RGB_COLOUR(pixels,x,y);
                    top = PIXEL_RGB_COLOUR(top_pixels,x-x_offset,y-y_offset);

                    if( add_flag )
                    {
                        r = get_Colour_r(bottom) + get_Colour_r(top);
                        g = get_Colour_g(bottom) + get_Colour_g(top);
                        b = get_Colour_b(bottom) + get_Colour_b(top);
                        a = get_Colour_a(bottom) + get_Colour_a(top);

                        if( r > 255 )  r = 255;
                        if( g > 255 )  g = 255;
                        if( b > 255 )  b = 255;
                        if( a > 255 )  a = 255;

                        col = make_rgba_Colour( r, g, b, a );
                    }
                    else
                    {
                        COMPOSITE_COLOURS( col, top, bottom );
                    }

                    PIXEL_RGB_COLOUR(pixels,x,y) = col;
                }
            }
            delete_pixels( &top_pixels );
        }

        ++n_files;
    }

    if( output_rgb_file( output_filename, &pixels ) != OK )
        return( 1 );

    return( 0 );
}
