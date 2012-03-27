#include  <volume_io.h>
#include  <bicpl/images.h>

private  int  paint_fill(
    pixels_struct   *pixels,
    int             x_seed,
    int             y_seed,
    Real            r_min,
    Real            r_max,
    Real            g_min,
    Real            g_max,
    Real            b_min,
    Real            b_max,
    Colour          new_colour );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING         input_filename, output_filename;
    Real           r_min, r_max, g_min, g_max, b_min, b_max;
    Real           r_new, g_new, b_new;
    int            x_seed, y_seed, n_changed;
    pixels_struct  pixels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &r_min ) ||
        !get_real_argument( 0.0, &r_max ) ||
        !get_real_argument( 0.0, &g_min ) ||
        !get_real_argument( 0.0, &g_max ) ||
        !get_real_argument( 0.0, &b_min ) ||
        !get_real_argument( 0.0, &b_max ) ||
        !get_real_argument( 0.0, &r_new ) ||
        !get_real_argument( 0.0, &g_new ) ||
        !get_real_argument( 0.0, &b_new ) ||
        !get_int_argument( 0, &x_seed ) ||
        !get_int_argument( 0, &y_seed ) )
    {
        print_error( "Usage: %s  input.rgb output.rgb\n", argv[0] );
        print_error( "           r1 r2 g1 g2 b1 b2 r g b x y\n" );
        return( 1 );
    }

    if( input_rgb_file( input_filename, &pixels ) != OK )
        return( 1 );

    if( pixels.pixel_type != RGB_PIXEL )
    {
        print( "Pixels must be RGB type.\n" );
        return( 1 );
    }

    n_changed = paint_fill( &pixels, x_seed, y_seed,
                            r_min, r_max,
                            g_min, g_max,
                            b_min, b_max,
                            make_Colour_0_1(r_new,g_new,b_new) );

    print( "Changed: %d\n", n_changed );

    if( output_rgb_file( output_filename, &pixels ) != OK )
        return( 1 );

    return( 0 );
}

private  BOOLEAN  should_change(
    pixels_struct   *pixels,
    Real            r_min,
    Real            r_max,
    Real            g_min,
    Real            g_max,
    Real            b_min,
    Real            b_max,
    int             x,
    int             y )
{
    Colour   col;
    Real     r, g, b;

    col = PIXEL_RGB_COLOUR(*pixels,x,y);

    r = get_Colour_r_0_1( col );
    g = get_Colour_g_0_1( col );
    b = get_Colour_b_0_1( col );

    return( r_min <= r && r <= r_max &&
            g_min <= g && g <= g_max &&
            b_min <= b && b <= b_max );
}

typedef struct
{
    int  x, y;
} pixel_position;


private  int  paint_fill(
    pixels_struct   *pixels,
    int             x_seed,
    int             y_seed,
    Real            r_min,
    Real            r_max,
    Real            g_min,
    Real            g_max,
    Real            b_min,
    Real            b_max,
    Colour          new_colour )
{
    int                             x_size, y_size, n_changed;
    int                             x_neigh, y_neigh;
    int                             x, y;
    int                             dir, n_dirs, *dx, *dy;
    Smallest_int                    **done_flags;
    pixel_position                  entry;
    QUEUE_STRUCT( pixel_position )  queue;

    x_size = pixels->x_size;
    y_size = pixels->y_size;

    if( x_seed < 0 || x_seed >= pixels->x_size ||
        y_seed < 0 || y_seed >= pixels->y_size )
    {
        print_error( "paint_fill: %d %d seed point out of range.\n" );
        return( 0 );
    }

    if( !should_change( pixels, r_min, r_max, g_min, g_max,
                                 b_min, b_max, x_seed, y_seed ) )
    {
        Colour  col;

 
        col = PIXEL_RGB_COLOUR(*pixels,x_seed,y_seed);

        print( "Image[%d,%d] = %g %g %g\n", x_seed, y_seed,
               get_Colour_r_0_1(col),
               get_Colour_g_0_1(col),
               get_Colour_b_0_1(col) );
        return( 0 );
    }

    n_dirs = get_neighbour_directions( EIGHT_NEIGHBOURS, &dx, &dy );

    INITIALIZE_QUEUE( queue );

    ALLOC2D( done_flags, x_size, y_size );

    for_less( x, 0, x_size )
        for_less( y, 0, y_size )
            done_flags[x][y] = FALSE;

    done_flags[x_seed][y_seed] = TRUE;
    PIXEL_RGB_COLOUR(*pixels,x_seed,y_seed) = new_colour;
    n_changed = 1;

    entry.x = x_seed;
    entry.y = y_seed;
    INSERT_IN_QUEUE( queue, entry );

    while( !IS_QUEUE_EMPTY( queue ) )
    {

        REMOVE_FROM_QUEUE( queue, entry );

        x = entry.x;
        y = entry.y;

        for_less( dir, 0, n_dirs )
        {
            x_neigh = x + dx[dir];
            y_neigh = y + dy[dir];

            if( x_neigh >= 0 && x_neigh < x_size &&
                y_neigh >= 0 && y_neigh < y_size &&
                !done_flags[x_neigh][y_neigh] )
            {
                done_flags[x_neigh][y_neigh] = TRUE;

                if( should_change( pixels, r_min, r_max, g_min, g_max,
                                   b_min, b_max, x_neigh, y_neigh ) )
                {
                    PIXEL_RGB_COLOUR(*pixels,x_neigh,y_neigh) = new_colour;

                    entry.x = x_neigh;
                    entry.y = y_neigh;
                    INSERT_IN_QUEUE( queue, entry );
                    ++n_changed;
                }
            }
        }
    }

    DELETE_QUEUE( queue );

    FREE2D( done_flags );

    return( n_changed );
}
