#include  <internal_volume_io.h>
#include  <images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING            input_filename, colour_name, output_filename;
    int               i, x, y, x_min, x_max, y_min, y_max, x_size, y_size;
    int               image_x_min, image_x_max, image_y_min, image_y_max;
    int               x_pos, y_pos, *x_poses, *y_poses, n_images;
    pixels_struct     in_pixels, *images, pixels;
    Colour            background;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &colour_name ) )
    {
        print_error( "Usage: %s output.rgb  bg_colour input1.rgb x1 y1 input2.rgb x2 y2...\n", argv[0] );
        return( 1 );
    }

    background = convert_string_to_colour( colour_name );

    images = NULL;
    x_poses = NULL;
    y_poses = NULL;

    n_images = 0;
    while( get_string_argument( NULL, &input_filename ) &&
           get_int_argument( 0, &x_pos ) &&
           get_int_argument( 0, &y_pos ) )
    {
        if( input_rgb_file( input_filename, &in_pixels ) != OK )
            return( 1 );

        ADD_ELEMENT_TO_ARRAY( images, n_images, in_pixels, DEFAULT_CHUNK_SIZE );
        --n_images;
        ADD_ELEMENT_TO_ARRAY( x_poses, n_images, x_pos, DEFAULT_CHUNK_SIZE );
        --n_images;
        ADD_ELEMENT_TO_ARRAY( y_poses, n_images, y_pos, DEFAULT_CHUNK_SIZE );
    }

    image_x_min = 0;
    image_x_max = 0;
    image_y_min = 0;
    image_y_max = 0;

    for_less( i, 0, n_images )
    {
        x_min = x_poses[i];
        x_max = x_poses[i] + images[i].x_size - 1;
        y_min = y_poses[i];
        y_max = y_poses[i] + images[i].y_size - 1;

        if( x_min < image_x_min )
            image_x_min = x_min;
        if( x_max > image_x_max )
            image_x_max = x_max;
        if( y_min < image_y_min )
            image_y_min = y_min;
        if( y_max > image_y_max )
            image_y_max = y_max;
    }

    x_size = image_x_max - image_x_min + 1;
    y_size = image_y_max - image_y_min + 1;

    initialize_pixels( &pixels, 0, 0, x_size, y_size, 1.0, 1.0, RGB_PIXEL );

    for_less( x, 0, x_size )
    for_less( y, 0, y_size )
        PIXEL_RGB_COLOUR( pixels, x, y ) = background;

    for_less( i, 0, n_images )
    {
        for_less( x, 0, images[i].x_size )
        for_less( y, 0, images[i].y_size )
            PIXEL_RGB_COLOUR( pixels, x + x_poses[i] - image_x_min,
                                      y + y_poses[i] - image_y_min ) =
                            PIXEL_RGB_COLOUR( images[i], x, y );
    }

    if( output_rgb_file( output_filename, &pixels ) != OK )
        return( 1 );

    return( 0 );
}
