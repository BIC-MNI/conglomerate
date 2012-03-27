#include  <volume_io.h>
#include  <bicpl.h>

private  BOOLEAN  get_next_filename(
    STRING      *filename )
{
    static    FILE     *file = 0;
    static    BOOLEAN  in_list = FALSE;
    static    STRING   filename_string;
    STRING             argument;
    BOOLEAN            found;

    found = FALSE;

    do
    {
        if( in_list )
        {
            if( input_string( file, &filename_string, ' ' ) == OK )
            {
                *filename = filename_string;
                found = TRUE;
            }
            else
            {
                (void) close_file( file );
                in_list = FALSE;
            }
        }
        else if( get_string_argument( "", &argument ) )
        {
            if( filename_extension_matches( argument,
                                get_default_landmark_file_suffix() )||
                filename_extension_matches( argument,
                                get_default_tag_file_suffix() ) ||
                filename_extension_matches( argument, "obj" ) )
            {
                *filename = create_string( argument );
                found = TRUE;
            }
            else
            {
                if( open_file( argument, READ_FILE, ASCII_FORMAT, &file ) != OK)
                    break;
                in_list = TRUE;
            }
        }
        else
            break;
    }
    while( !found );

    return( found );
}

public  int  get_filename_arguments(
    STRING   *filenames[] )
{
    STRING    filename;
    int       n_files;

    n_files = 0;

    while( get_next_filename( &filename ) )
    {
        SET_ARRAY_SIZE( *filenames, n_files, n_files + 1, DEFAULT_CHUNK_SIZE );
        (*filenames)[n_files] = filename;
        ++n_files;
    }

    return( n_files );
}
