#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s output.txt input1 input2 input3\n" );
        return( 0 );
    }

    n_samples = 0;
    n_points = 0;

    while( get_string_argument( NULL, &filename ) )
    {
        ++n_samples;
    }
}
