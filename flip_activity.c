#include  <def_mni.h>

#define UK

#ifdef UK
private  int  sizes[] = { 256, 124, 172 };
#else
private  int  sizes[] = { 256, 256, 80 };
#endif

int  main(
    int   argc,
    char  *argv[] )
{
    Status              status;
    char                *input_filename, *output_filename;
    int                 x, y, z;
    bitlist_3d_struct   input_bitlist, output_bitlist;
    FILE                *file;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        (void) fprintf( stderr, "Arguments?\n" );
        exit( 1 );
    }

    create_bitlist_3d( sizes[0], sizes[1], sizes[2], &input_bitlist );

#ifdef UK
    create_bitlist_3d( sizes[0], sizes[1], sizes[2], &output_bitlist );
#else
    create_bitlist_3d( sizes[0], sizes[1], sizes[2], &output_bitlist );
#endif

    status = open_file( input_filename, READ_FILE, BINARY_FORMAT, &file );

    if( status == OK )
        status = io_bitlist_3d( file, READ_FILE, &input_bitlist );

    if( status == OK )
        status = close_file( file );

    for_less( x, 0, sizes[0] )
    for_less( y, 0, sizes[1] )
    for_less( z, 0, sizes[2] )
    {
#ifdef UK
        if( get_bitlist_bit_3d( &input_bitlist, x, y, z ) )
            set_bitlist_bit_3d( &output_bitlist, x, y, sizes[2]-1-z, TRUE );
#else
        if( get_bitlist_bit_3d( &input_bitlist, x, y, z ) )
            set_bitlist_bit_3d( &output_bitlist, x, sizes[1]-1-y, z, TRUE );
#endif
    }

    if( status == OK )
        status = open_file( output_filename, WRITE_FILE, BINARY_FORMAT, &file );

    if( status == OK )
        status = io_bitlist_3d( file, WRITE_FILE, &output_bitlist );

    if( status == OK )
        status = close_file( file );

    return( status != OK );
}
