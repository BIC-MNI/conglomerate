#include  <mni.h>

int main(
    int   argc,
    char  *argv[] )
{
    VIO_Status  status;
    int     s;
    VIO_STR  str, prev_str, prev_dir, dir;
    VIO_BOOL last_same, same;
    FILE    *in_file, *out_file;
    char    *input_filename, *output_filename;

    initialize_argument_processing( argc, argv );

    if( get_string_argument( "", &input_filename ) )
        status = open_file( input_filename, READ_FILE, ASCII_FORMAT, &in_file );
    else
        in_file = stdin;

    if( get_string_argument( "", &output_filename ) )
        status = open_file( output_filename, WRITE_FILE, ASCII_FORMAT,
                            &out_file );
    else
        out_file = stdout;

    last_same = TRUE;
    prev_dir[0] = (char) 0;

    while( input_string( in_file, str, MAX_STRING_LENGTH, ' ' ) == VIO_OK )
    {
        strcpy( dir, str );
        s = strlen(dir);
        while( s > 0 && dir[s] != '/' )
            --s;
        dir[s] = (char) 0;

        same = strcmp( prev_dir, dir ) == 0;

        if( last_same && same )
            print( "Third file for same patient %s\n", dir );
        else if( last_same && !same )
            last_same = FALSE;
        else if( !last_same && same )
        {
            status = output_string( out_file, prev_str );
            status = output_newline( out_file );
            status = output_string( out_file, str );
            status = output_newline( out_file );
            last_same = TRUE;
        }

        (void) strcpy( prev_str, str );
        (void) strcpy( prev_dir, dir );
    }

    return( status != VIO_OK );
}
