#include  <volume_io.h>
#include  <bicpl/images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR            input_filename, output_filename;
    int               n_neighbours;
    int               x, y, dir, n_dirs, *dx, *dy, tx, ty;
    Neighbour_types   conn;
    pixels_struct     in_pixels, out_pixels;
    Colour            col;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s input.rgb output.rgb  [4|8]\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 8, &n_neighbours );

    if( n_neighbours == 4 )
        conn = FOUR_NEIGHBOURS;
    else
        conn = EIGHT_NEIGHBOURS;

    if( input_rgb_file( input_filename, &in_pixels ) != VIO_OK )
        return( 1 );

    if( in_pixels.pixel_type != RGB_PIXEL )
    {
        print( "Pixels must be RGB type.\n" );
        return( 1 );
    }

    initialize_pixels( &out_pixels, 0, 0, in_pixels.x_size, in_pixels.y_size,
                       1.0, 1.0, RGB_PIXEL );

    n_dirs = get_neighbour_directions( conn, &dx, &dy );

    for_less( x, 0, in_pixels.x_size )
    {
        for_less( y, 0, in_pixels.y_size )
        {
            for_less( dir, 0, n_dirs )
            {
                tx = x + dx[dir];
                ty = y + dy[dir];

                if( tx >= 0 && tx < in_pixels.x_size &&
                    ty >= 0 && ty < in_pixels.y_size )
                {
                    col = PIXEL_RGB_COLOUR(in_pixels,tx,ty);

                    if( col == BLACK )
                        break;
                    else if( col != WHITE )
                        print( "%d %d %d\n", get_Colour_r(col),
                                             get_Colour_g(col),
                                             get_Colour_b(col) );
                }
            }

            if( dir != n_dirs )
                PIXEL_RGB_COLOUR(out_pixels,x,y) = BLACK;
            else
                PIXEL_RGB_COLOUR(out_pixels,x,y) = WHITE;
        }
    }

    if( output_rgb_file( output_filename, &out_pixels ) != VIO_OK )
        return( 1 );

    return( 0 );
}
