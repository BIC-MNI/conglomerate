#include  <bicpl.h>
#include  <volume_io.h>

FILE  *file;
FILE  *in_file;

static   int  n_on = 0;
static   int  total_bytes_read = 0;

private  VIO_BOOL  get_bit(
    VIO_BOOL  *bit )
{
    static  int   n_bits_left = 0;
    static  int   data;

    if( n_bits_left == 0 )
    {
        data = fgetc( in_file );
        if( data == EOF )
            return( FALSE );
        n_bits_left = 8;
        ++total_bytes_read;
    }

    --n_bits_left;
    *bit = (data & 1) != 0;
    data >>= 1;

    if( *bit ) ++n_on;

    return( TRUE );
}

static  int   total_n_bits = 0;
static  int   n_bits_out = 0;
static  int   data = 0;

private  void  output_bit(
    VIO_BOOL  bit )
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
    print( "Total bits: %d %d   %5.1f\n", total_bytes_read, total_n_bits / 8,
           100.0 * (VIO_Real) n_on / 8.0 / (VIO_Real) total_bytes_read );

    while( n_bits_out != 0 )
        output_bit( FALSE );
}

int  main(
    int  argc,
    char *argv[] )
{
    VIO_STR     bit_pattern, out_filename, in_filename;
    VIO_Real       total_bits, l;
    VIO_BOOL    bit, expected;
    int        n_pattern_bits, *count, n_patterns, b, p, value;
    int        smallest, second_smallest, total_chunks;

    initialize_argument_processing( argc, argv );

    if( !get_int_argument( 0, &n_pattern_bits ) ||
        !get_string_argument( NULL, &in_filename ) ||
        !get_string_argument( NULL, &out_filename ) )
    {
        print_error( "Usage: %s pattern out", argv[0] );
        return( 1 );
    }

    n_patterns = 1 << n_pattern_bits;
    l = log( (VIO_Real) (n_patterns-1) ) / log( 2.0 );

    if( open_file( in_filename, READ_FILE, BINARY_FORMAT, &in_file ) != VIO_OK )
        return( 1 );

    ALLOC( count, n_patterns );
    for_less( p, 0, n_patterns )
        count[p] = 0;

    while( 1 )
    {
        value = 0;
        for_less( b, 0, n_pattern_bits )
        {
            if( !get_bit( &bit ) )
                break;

            value = (value << 1) | bit;
        }

        if( b < n_pattern_bits )
            break;

        ++count[value];
    }

    close_file( in_file );

    if( count[0] < count[1] )
        smallest = 0;
    else
        smallest = 1;
    second_smallest = 1 - smallest;

    for_less( p, 2, n_patterns )
    {
        if( count[p] < smallest )
        {
            second_smallest = smallest;
            smallest = count[p];
        }
        else if( count[p] < second_smallest )
            second_smallest = count[p];
    }

    total_chunks = 0;
    for_less( p, 0, n_patterns )
    {
         print( "%d %d\n", p, count[p] );
         total_chunks += count[p];
    }

    print( "Smallest: %d %d\n", smallest, count[smallest] );
    print( "Second Smallest: %d %d\n", second_smallest, count[second_smallest] );

    total_bits = 0.0;
    for_less( p, 0, n_patterns )
        total_bits += (VIO_Real) count[p] * l;

    total_bits += (VIO_Real) count[smallest] +
                  (VIO_Real) count[second_smallest];

    print( "Percentage: %g %g", 2.0 / (VIO_Real) n_patterns,
            ((VIO_Real) count[smallest] + (VIO_Real) count[second_smallest]) /
            (VIO_Real) total_chunks );

    print( "Ratio: %g\n", total_bits / (VIO_Real) total_bytes_read / 8.0 );

/*
    if( open_file( out_filename, WRITE_FILE, BINARY_FORMAT, &file ) != VIO_OK )
        return( 1 );

    close_file( file );
*/

    return( 0 );
}
