#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: ascii_binary  input.obj  [output.obj] [ascii|binary]\n\
\n\
     Converts ascii .obj files to binary and vice versa, placing output in\n\
     either output.obj, if specified, or input.obj.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING         input_filename, output_filename, format_type;
    int            n_objects;
    File_formats   format, file_format;
    object_struct  **object_list;
    BOOLEAN        format_specified;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );
    format_specified = get_string_argument( NULL, &format_type );

    if( format_specified )
    {
        if( format_type[0] == 'a' || format_type[0] == 'A' )
            format = ASCII_FORMAT;
        else
            format = BINARY_FORMAT;
    }
    
    if( filename_extension_matches(input_filename,
                                   get_default_tag_file_suffix()) )
    {
        if( input_objects_any_format( NULL, input_filename,
                                      GREEN, 1.0, SPHERE_MARKER,
                                      &n_objects, &object_list ) != OK )
            return( 1 );

        file_format = BINARY_FORMAT;
    }
    else
    {
        if( input_graphics_file( input_filename, &file_format,
                                 &n_objects, &object_list ) != OK )
            return( 1 );
    }

    if( !format_specified )
    {
        if( file_format == BINARY_FORMAT )
            format = ASCII_FORMAT;
        else
            format = BINARY_FORMAT;
    }

    (void) output_graphics_file( output_filename, format,
                                 n_objects, object_list );

    delete_object_list( n_objects, object_list );

    return( 0 );
}
