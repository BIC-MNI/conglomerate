#include  <mni.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    File_formats         format;
    int                  i, n_objects;
    object_struct        **objects;
    pixels_struct        *pixels, pixels_rgb;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  input.obj  output.rgb\n",
               argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input_filename,
                             &format, &n_objects, &objects ) != VIO_OK )
        return( 1 );

    if( n_objects != 1 || get_object_type( objects[0] ) != PIXELS )
    {
        print( "Must have one pixels object.\n" );
        return( 1 );
    }

    pixels = get_pixels_ptr( objects[0] );

    convert_index8_to_pixels24( pixels, get_8bit_rgb_pixel_lookup(),
                                &pixels_rgb );

    (void) output_rgb_file( output_filename, &pixels_rgb );

    delete_object_list( n_objects, objects );

    return( 0 );
}
