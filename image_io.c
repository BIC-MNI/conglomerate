#include <internal_volume_io.h>
#include <bicpl.h>

public  Status  get_image_file_size(
    STRING         filename,
    int            *x_size,
    int            *y_size,
    int            *nc )
{
    char   command[EXTREMELY_LARGE_STRING_SIZE+1];
    int    z_size;
    STRING line;
    FILE   *file;

    /*---  get the image size */

    (void) sprintf( command, "imginfo %s", filename );

    if( (file = popen( command, "r")) == NULL )
    {
        print_error( "Error opening file %s or finding command 'imginfo'\n",
                     filename );
        return( ERROR );
    }

    if( input_line( file, &line ) != OK )
        return( ERROR );

    delete_string( line );

    if( input_line( file, &line ) != OK )
        return( ERROR );

    pclose( file );

    if( sscanf( line, "Dimensions (x,y,z,c):	%d, %d, %d, %d",
                x_size, y_size, &z_size, nc ) != 4 )
    {
        print_error( "Error reading size from file %s\n", filename );
        return( ERROR );
    }

    if( z_size != 1 )
    {
        print_error( "Cannot handle case where z != 1 in input %s\n",
                     filename );
        return( ERROR );
    }

    return( OK );
}

public  Status  read_image_file(
    STRING         filename,
    pixels_struct  *pixels )
{
    char           command[EXTREMELY_LARGE_STRING_SIZE+1];
    int            x_size, y_size, nc, x, y;
    FILE           *file;
    unsigned char  colour[4];
    Colour         col;

    if( get_image_file_size( filename, &x_size, &y_size, &nc ) != OK )
        return( ERROR );

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
        return( ERROR );
    }

    for_less( y, 0, y_size )
    {
        for_less( x, 0, x_size )
        {
            if( io_binary_data( file, READ_FILE, &colour, sizeof(colour[0]),
                                nc ) != OK )
            {
                print( "Error reading pixel[%d][%d]\n", x, y );
                return( ERROR );
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

    return( OK );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING          input_filename; 
    pixels_struct   pixels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) )
    {
        print_error( "Need an argument.\n" );
        return( 1 );
    }

    (void) read_image_file( input_filename, &pixels );

    return( 0 );
}
