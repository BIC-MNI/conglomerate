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
                                           ASCII_FORMAT, &input ) != OK )
        {
            return( 1 );
        }
    }
    else
        input = stdin;

    if( get_string_argument( "", &output_filename ) )
    {
        if( open_file_with_default_suffix( output_filename, "lmk", WRITE_FILE,
                                           ASCII_FORMAT, &output ) != OK )
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

private  BOOLEAN  input_tag_point(
    FILE   *file,
    Real   *xi,
    Real   *yi,
    Real   *zi,
    Real   *xt,
    Real   *yt,
    Real   *zt,
    Real   *t,
    Real   *z,
    int    *id,
    char   label[] )
{
    BOOLEAN  okay;

    okay = input_real( file, xi ) == OK &&
           input_real( file, yi ) == OK &&
           input_real( file, zi ) == OK &&
           input_real( file, xt ) == OK &&
           input_real( file, yt ) == OK &&
           input_real( file, zt ) == OK &&
           input_real( file, t ) == OK &&
           input_real( file, z ) == OK &&
           input_int( file, id ) == OK;

    if( okay )
        (void) input_line( file, label, MAX_STRING_LENGTH );

    return( okay );
}

private  void  output_tag_point(
    FILE   *file,
    Real   xt,
    Real   yt,
    Real   zt,
    Real   t,
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
    Real    xi, yi, zi, xt, yt, zt, t, z;
    int     id;
    STRING  label;

    while( input_tag_point( input, &xi, &yi, &zi, &xt, &yt, &zt,
                            &t, &z, &id, label ) )
    {
        output_tag_point( output, xt, yt, zt, t, id, label );
    }
}
