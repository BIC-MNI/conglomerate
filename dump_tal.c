#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    char           *input_filename;
    int            x, y, n_objects;
    Colour         col;
    File_formats   format;
    object_struct  **object_list;
    pixels_struct  *pixels, pixels_rgb;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list ) != VIO_OK )
        return( 1 );

    if( n_objects != 1 || get_object_type( object_list[0] ) != PIXELS )
        return( 1 );

    pixels = get_pixels_ptr( object_list[0] );

    convert_index8_to_pixels24( pixels, get_8bit_rgb_pixel_lookup(),
                                &pixels_rgb );

    delete_pixels( pixels );

    convert_pixels24_to_gray_scale( &pixels_rgb, pixels );

    (void) fprintf( stderr, "%d %d\n", pixels->x_size, pixels->y_size );

    for_less( y, 0, pixels->y_size )
    {
        for( x = pixels->x_size-1;  x >= 0;  --x )
        {
            col = PIXEL_COLOUR_INDEX_8( *pixels, x, y );
            print( "%c", col );
        }
    }

    return( 0 );
}
