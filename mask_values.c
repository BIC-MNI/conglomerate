#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input1.mnc input2.mnc output.mnc min1 max1 set2\n\
           \n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_Real        *values1, *values2, min1, max1, set2;
    int         n_values1, n_values2, i;
    VIO_STR      input1_filename, input2_filename, dest_filename;
    VIO_File_formats format;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_real_argument( 0.0, &min1 ) ||
        !get_real_argument( 0.0, &max1 ) ||
        !get_real_argument( 0.0, &set2 ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_texture_values( input1_filename, &n_values1, &values1 ) != OK ||
        input_texture_values( input2_filename, &n_values2, &values2 ) != OK ||
        n_values1 != n_values2 )
    {
        print_error( "Error in values.\n" );
        return( 1 );
    }

    for_less( i, 0, n_values1 )
    {
        if( values1[i] >= min1 && values1[i] <= max1 )
            values2[i] = set2;
    }

    if( string_ends_in( dest_filename, ".mnc" ) )
        format = BINARY_FORMAT;
    else
        format = ASCII_FORMAT;
    (void) output_texture_values( dest_filename, format,
                                  n_values2, values2 );

    return( 0 );
}
