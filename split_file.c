#include <internal_volume_io.h>
#include <bicpl.h>

int  main(
    int  argc,
    char *argv[] )
{
    int     n_bytes, file_number, ch, n_bytes_per_file;
    STRING  output_prefix;
    char    filename[10000];
    FILE    *file;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_prefix ) )
    {
        print_error( "Usage: %s  output_prefix  [max_bytes_per_file]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1000000, &n_bytes_per_file );

    n_bytes = 0;
    file_number = 1;

    (void) sprintf( filename, "%s.%03d", output_prefix, file_number );

    if( open_file( filename, WRITE_FILE, BINARY_FORMAT, &file ) != OK )
        return( 1 );

    while( (ch = getchar()) != EOF )
    {
        putc( ch, file );
        ++n_bytes;

        if( n_bytes == n_bytes_per_file )
        {
            (void) close_file( file );

            n_bytes = 0;
            ++file_number;

            (void) sprintf( filename, "%s.%03d", output_prefix, file_number );

            if( open_file( filename, WRITE_FILE, BINARY_FORMAT, &file ) != OK )
                return( 1 );
        }
    }

    (void) close_file( file );

    return( 0 );
}
