#include  <bicpl.h>
#include  <internal_volume_io.h>

FILE  *file;

static   int  n_on = 0;
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

    if( *bit ) ++n_on;

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
    print( "Total bits: %d %d   %5.1f\n", total_bytes_read, total_n_bits / 8,
           100.0 * (Real) n_on / 8.0 / (Real) total_bytes_read );

    while( n_bits_out != 0 )
        output_bit( FALSE );
}

int  main(
    int  argc,
    char *argv[] )
{
    STRING     bit_pattern, out_filename;
    Real       prob_on;
    BOOLEAN    bit, expected;
    int        seed, n_correct;

    initialize_argument_processing( argc, argv );

    if( !get_real_argument( 0.0, &prob_on ) ||
        !get_string_argument( NULL, &out_filename ) )
    {
        print_error( "Usage: %s pattern out", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 341424341, &seed );

    set_random_seed( seed );

    if( open_file( out_filename, WRITE_FILE, BINARY_FORMAT, &file ) != OK )
        return( 1 );

    n_correct = 0;

    while( get_bit( &bit ) )
    {
        expected = get_random_0_to_1() < prob_on;

        if( bit == expected )
            ++n_correct;

        if( n_correct == 2 || bit != expected && n_correct > 0 )
        {
            if( n_correct == 2 )
            {
                output_bit( 1 );
            }
            else if( n_correct == 1 )
            {
                output_bit( 0 );
                output_bit( 1 );
            }
        }

        if( bit != expected && (n_correct == 0 || n_correct == 2) )
        {
            output_bit( 0 );
            output_bit( 0 );
        }

        if( n_correct == 2 || bit != expected && n_correct > 0 )
            n_correct = 0;
    }

    flush_bits();

    close_file( file );

    return( 0 );
}
