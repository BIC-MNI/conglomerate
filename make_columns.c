#include  <volume_io.h>

int  main(
    int  argc,
    char *argv[] )
{
    FILE    *file;
    int     i, j, n_files;
    float   **values;
    VIO_Real    value;

    n_files = argc - 1;

    ALLOC2D( values, n_files, 65538 );

    for_less( i, 0, n_files )
    {
        if( open_file( argv[i+1], READ_FILE, ASCII_FORMAT, &file ) != VIO_OK )
            return( 1 );

        for_less( j, 0, 65538 )
        {
            if( input_real( file, &value ) != VIO_OK )
                return( 1 );
            values[i][j] = (float) value;
        }

        close_file( file );
    }

    for_less( j, 0, 65538 )
    {
        for_less( i, 0, n_files )
            print( " %g", values[i][j] );
        print( "\n" );
    }

    return( 0 );
}
