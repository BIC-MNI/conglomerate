#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char           *input_filename, *swap_bytes_string;
    char           one_byte, last_byte;
    int            i, offset, n_bytes;
    BOOLEAN        swap_bytes_flag;
    FILE           *file;

    initialize_argument_processing( argc, argv );

    if( !get_int_argument( 0, &offset ) ||
        !get_int_argument( -1, &n_bytes ) ||
        !get_string_argument( "", &swap_bytes_string ) )
    {
        print( "Usage: %s  offset  n_bytes|-1 swap/noswap  file1  file2 ...\n",
               argv[0] );
        return( 1 );
    }

    swap_bytes_flag = strcmp( swap_bytes_string, "swap" ) == 0;

    while( get_string_argument( "", &input_filename ) )
    {
        if( open_file( input_filename, READ_FILE, BINARY_FORMAT, &file ) != OK )
            return( 1 );

        set_file_position( file, offset );

        i = 0;
        while( (n_bytes < 0 || i < n_bytes) &&
               input_character( file, &one_byte ) == OK )
        {
            if( swap_bytes_flag )
            {
                if( (i % 2) == 0 )
                    last_byte = one_byte;
                else
                {
                    if( output_character( stdout, one_byte ) != OK )
                    {
                        print_error( "Error writing byte.\n" );
                        return( 1 );
                    }
                    if( output_character( stdout, last_byte ) != OK )
                    {
                        print_error( "Error writing byte.\n" );
                        return( 1 );
                    }
                }
            }
            else if( output_character( stdout, one_byte ) != OK )
            {
                print_error( "Error writing byte.\n" );
                return( 1 );
            }

            ++i;
        }

        (void) close_file( file );
    }

    return( 0 );
}
