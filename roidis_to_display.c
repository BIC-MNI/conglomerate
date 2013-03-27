#include  <def_mni.h>

private  void  convert_file( FILE *, FILE * );

int  main(
    int   argc,
    char  *argv[] )
{
    char     *input_filename, *output_filename;
    FILE     *input, *output;

    initialize_argument_processing( argc, argv );

    if( get_string_argument( "", &input_filename ) )
    {
        if( open_file_with_default_suffix( input_filename, "cnt", READ_FILE,
                                           ASCII_FORMAT, &input ) != VIO_OK )
        {
            return( 1 );
        }
    }
    else
        input = stdin;

    if( get_string_argument( "", &output_filename ) )
    {
        if( open_file_with_default_suffix( output_filename, "lmk", WRITE_FILE,
                                           ASCII_FORMAT, &output ) != VIO_OK )
        {
            return( 1 );
        }
    }
    else
        output = stdout;

    convert_file( input, output );

    if( input != stdin )
        (void) close_file( input );

    if( output != stdout )
        (void) close_file( output );

    return( 0 );
}

private  VIO_BOOL  input_tag_point(
    FILE   *file,
    VIO_Real   *xi,
    VIO_Real   *yi,
    VIO_Real   *zi,
    VIO_Real   *xt,
    VIO_Real   *yt,
    VIO_Real   *zt,
    VIO_Real   *t,
    VIO_Real   *z,
    int    *id,
    char   label[] )
{
    VIO_BOOL  okay;

    okay = input_real( file, xi ) == VIO_OK &&
           input_real( file, yi ) == VIO_OK &&
           input_real( file, zi ) == VIO_OK &&
           input_real( file, xt ) == VIO_OK &&
           input_real( file, yt ) == VIO_OK &&
           input_real( file, zt ) == VIO_OK &&
           input_real( file, t ) == VIO_OK &&
           input_real( file, z ) == VIO_OK &&
           input_int( file, id ) == VIO_OK;

    if( okay )
        (void) input_line( file, label, MAX_STRING_LENGTH );

    return( okay );
}

private  void  output_tag_point(
    FILE   *file,
    VIO_Real   xt,
    VIO_Real   yt,
    VIO_Real   zt,
    VIO_Real   t,
    int    id,
    char   label[] )
{
    convert_mm_to_talairach( xt, yt, zt, &xt, &yt, &zt );

    (void) output_real( file, xt );
    (void) output_real( file, yt );
    (void) output_real( file, zt );
    (void) output_real( file, t );
    (void) output_int( file, id );
    (void) output_int( file, 0 );
    (void) output_string( file, label );
    (void) output_newline( file );
}

private  void  convert_file(
    FILE   *input,
    FILE   *output )
{
    VIO_Real    xi, yi, zi, xt, yt, zt, t, z;
    int     id;
    VIO_STR  label;

    while( input_tag_point( input, &xi, &yi, &zi, &xt, &yt, &zt,
                            &t, &z, &id, label ) )
    {
        output_tag_point( output, xt, yt, zt, t, id, label );
    }
}
