#include  <volume_io.h>
#include  <bicpl.h>

static  VIO_BOOL  get_next_filename(
    VIO_STR      *filename )
{
    static    FILE     *file = 0;
    static    VIO_BOOL  in_list = FALSE;
    static    VIO_STR   filename_string;
    VIO_STR             argument;
    VIO_BOOL            found;

    found = FALSE;

    do
    {
        if( in_list )
        {
            if( input_string( file, &filename_string, ' ' ) == VIO_OK )
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
                if( open_file( argument, READ_FILE, ASCII_FORMAT, &file ) != VIO_OK)
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

  int  get_filename_arguments(
    VIO_STR   *filenames[] )
{
    VIO_STR    filename;
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
