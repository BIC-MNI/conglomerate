#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    FILE        *file;
    int         n;
    Real        *x;
    Real        value, min, max, mean, std_dev, median;
    char        *filename;

    initialize_argument_processing( argc, argv );

    if( get_string_argument( "", &filename ) )
    {
        if( open_file( filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );
    }
    else
        file = stdin;

    n = 0;

    while( input_real( file, &value ) == OK )
    {
        ADD_ELEMENT_TO_ARRAY( x, n, value, DEFAULT_CHUNK_SIZE );
    }

    compute_statistics( n, x, &min, &max, &mean, &std_dev, &median );

    if( file != stdin )
        print( "--- Stats for %s, consisting of %d samples.\n", filename, n );
    else
        print( "--- Stats for %d samples.\n", n );

    print( "min: %g    max: %g\n", min, max );
    print( "median: %g   mean: %g    std dev: %g\n", median, mean, std_dev );

    if( file != stdin )
        (void) close_file( file );

    return( 0 );
}
