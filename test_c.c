#include  <bicpl.h>
#include  <internal_volume_io.h>

FILE  *file;

static   int  total_bytes_read = 0;

private  BOOLEAN  get_bit(
    BOOLEAN  *bit )
{
    static  int   n_bits_left = 0;
    static  int   data;

    if( n_bits_left == 0 )
    {
        data = getchar();
        if( data == EOF )
            return( FALSE );
        n_bits_left = 8;
        ++total_bytes_read;
    }

    --n_bits_left;
    *bit = (data & 1) != 0;
    data >>= 1;

    return( TRUE );
}

static  int   total_n_bits = 0;
static  int   n_bits_out = 0;
static  int   data = 0;

private  void  output_bit(
    BOOLEAN  bit )
{
    data = (data >> 1) | (bit ? 128 : 0);

    ++total_n_bits;;

    ++n_bits_out;

    if( n_bits_out == 8 )
    {
        fputc( data, file );
        data = 0;
        n_bits_out = 0;
    }
}

private  void  flush_bits( void )
{
    print( "Total bits: %d %d\n", total_bytes_read, total_n_bits / 8 );

    while( n_bits_out != 0 )
        output_bit( FALSE );
}

int  main(
    int  argc,
    char *argv[] )
{
    STRING     bit_pattern, out_filename;
    BOOLEAN    bit, m[2];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &bit_pattern ) ||
        !get_string_argument( NULL, &out_filename ) )
    {
        print_error( "Usage: %s pattern out", argv[0] );
        return( 1 );
    }

    if( open_file( out_filename, WRITE_FILE, BINARY_FORMAT, &file ) != OK )
        return( 1 );

    m[0] = bit_pattern[0] == '1';
    m[1] = bit_pattern[1] == '1';

    print( "%d %d\n", m[0], m[1] );

    while( get_bit( &bit ) )
    {
        if( bit == m[0] )
        {
            if( !get_bit( &bit ) )
                break;

            if( bit == m[1] )
                output_bit( TRUE );
            else
            {
                output_bit( FALSE );
                output_bit( TRUE );
            }
        }
        else
        {
            output_bit( FALSE );
            output_bit( FALSE );
        }
    }

    flush_bits();

    close_file( file );

    return( 0 );
}
