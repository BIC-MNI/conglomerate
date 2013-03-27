#include <volume_io.h>
#include <bicpl.h>

public  VIO_Status  get_image_file_size(
    VIO_STR         filename,
    int            *x_size,
    int            *y_size,
    int            *nc )
{
    char   command[EXTREMELY_LARGE_STRING_SIZE+1];
    FILE   *file;

    /*---  get the image size */

    (void) sprintf( command, "get_image_size.pl %s", filename );

    if( (file = popen( command, "r")) == NULL )
    {
        print_error( "Error opening file %s or finding command 'get_image_size.pl'\n",
                     filename );
        return( VIO_ERROR );
    }

    if( input_int( file, x_size ) != VIO_OK ||
        input_int( file, y_size ) != VIO_OK ||
        input_int( file, nc ) != VIO_OK )
        return( VIO_ERROR );

    pclose( file );

    return( VIO_OK );
}

public  VIO_Status  read_image_file(
    VIO_STR         filename,
    pixels_struct  *pixels )
{
    char           command[EXTREMELY_LARGE_STRING_SIZE+1];
    int            x_size, y_size, nc, x, y;
    FILE           *file;
    unsigned char  colour[4];
    Colour         col;

    if( get_image_file_size( filename, &x_size, &y_size, &nc ) != VIO_OK )
        return( VIO_ERROR );

    /*---  create the image with the correct size */

    initialize_pixels( pixels, 0, 0, x_size, y_size, 1.0, 1.0, RGB_PIXEL );

    /*---  input the image */

    if( nc == 3 )
        (void) sprintf( command, "convert %s RGB:-", filename );
    else
        (void) sprintf( command, "convert %s RGBA:-", filename );

    if( (file = popen( command, "r")) == NULL )
    {
        print_error( "Error opening file %s or finding command 'convert'\n",
                     filename );
        return( VIO_ERROR );
    }

    for_less( y, 0, y_size )
    {
        for_less( x, 0, x_size )
        {
            if( io_binary_data( file, READ_FILE, colour, sizeof(colour[0]),
                                nc ) != VIO_OK )
            {
                print( "Error reading pixel[%d][%d]\n", x, y );
                return( VIO_ERROR );
            }

            if( nc == 3 )
            {
                col = make_Colour( (int) colour[0], (int) colour[1],
                                   (int) colour[2] );
            }
            else
            {
                col = make_rgba_Colour( (int) colour[0], (int) colour[1],
                                        (int) colour[2], (int) colour[3] );
            }

            PIXEL_RGB_COLOUR(*pixels,x,y) = col;
        }
    }

    pclose( file );

    return( VIO_OK );
}

public  VIO_Status  write_image_file(
    VIO_STR         filename,
    pixels_struct  *pixels )
{
    char           command[EXTREMELY_LARGE_STRING_SIZE+1];
    int            x_size, y_size, x, y, r, g, b, a;
    FILE           *file;
    unsigned char  colour[4];
    Colour         col;

    x_size = pixels->x_size;
    y_size = pixels->y_size;

    if( find_character(filename,':') < 0 &&
        string_ends_in( filename, ".rgb" ) )
    {
        (void) sprintf( command, "convert -size %dx%d RGBA:- SGI:%s",
                        x_size, y_size, filename );
    }
    else
    {
        (void) sprintf( command, "convert -size %dx%d RGBA:- %s",
                        x_size, y_size, filename );
    }

    if( (file = popen( command, "w")) == NULL )
    {
        print_error( "Error opening file %s or finding command 'convert'\n",
                     filename );
        return( VIO_ERROR );
    }

    for_less( y, 0, y_size )
    {
        for_less( x, 0, x_size )
        {
            col = PIXEL_RGB_COLOUR(*pixels,x,y);

            r = get_Colour_r( col );
            g = get_Colour_g( col );
            b = get_Colour_b( col );
            a = get_Colour_a( col );

            colour[0] = (unsigned char) r;
            colour[1] = (unsigned char) g;
            colour[2] = (unsigned char) b;
            colour[3] = (unsigned char) a;

/*
            colour[0] = (unsigned char) ((VIO_Real) r * (VIO_Real) a / 255.0 + 0.5);
            colour[1] = (unsigned char) ((VIO_Real) g * (VIO_Real) a / 255.0 + 0.5);
            colour[2] = (unsigned char) ((VIO_Real) b * (VIO_Real) a / 255.0 + 0.5);
*/

            if( io_binary_data( file, WRITE_FILE, colour, sizeof(colour[0]),
                                4 ) != VIO_OK )
            {
                print( "Error writing pixel[%d][%d]\n", x, y );
                return( VIO_ERROR );
            }
        }
    }

    pclose( file );

    return( VIO_OK );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR          input_filename, output_filename;
    pixels_struct   pixels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  input output\n", argv[0] );
        return( 1 );
    }

    if( read_image_file( input_filename, &pixels ) != VIO_OK )
    {
        print_error( "Error on input.\n" );
        return( 1 );
    }

    if( write_image_file( output_filename, &pixels ) != VIO_OK )
    {
        print_error( "Error on output.\n" );
        return( 1 );
    }

    return( 0 );
}
