#include  <internal_volume_io.h>
#include  <bicpl.h>

public  Status  process_object(
    object_struct  *object );

private  void  usage(
    char   executable[] )
{
    char  *usage_str = "\n\
Usage: %s  input.obj  [output.obj]\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    Status         status;
    char           *input_filename, *output_filename;
    int            i, n_objects;
    File_formats   format;
    object_struct  **object_list;

    status = OK;

    if( argc == 1 )
    {
        usage( argv[0] );
        status = ERROR;
    }

    if( status == OK )
    {
        input_filename = argv[1];

        if( argc > 2 )
            output_filename = argv[2];
        else
            output_filename = input_filename;

        status = input_graphics_file( input_filename, &format, &n_objects,
                                      &object_list );

        if( status == OK )
            print( "Objects input.\n" );
    }

    if( status == OK )
    {
        for_less( i, 0, n_objects )
        {
            if( status == OK )
                status = process_object( object_list[i] );
        }

        if( status == OK )
            print( "Objects processed.\n" );
    }

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       n_objects, object_list );

    if( status == OK )
        delete_object_list( n_objects, object_list );

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}
