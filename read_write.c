#include  <volume_io.h>
#include  <bicpl.h>

public  VIO_Status  process_object(
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
    VIO_Status         status;
    char           *input_filename, *output_filename;
    int            i, n_objects;
    File_formats   format;
    object_struct  **object_list;

    status = VIO_OK;

    if( argc == 1 )
    {
        usage( argv[0] );
        status = VIO_ERROR;
    }

    if( status == VIO_OK )
    {
        input_filename = argv[1];

        if( argc > 2 )
            output_filename = argv[2];
        else
            output_filename = input_filename;

        status = input_graphics_file( input_filename, &format, &n_objects,
                                      &object_list );

        if( status == VIO_OK )
            print( "Objects input.\n" );
    }

    if( status == VIO_OK )
    {
        for_less( i, 0, n_objects )
        {
            if( status == VIO_OK )
                status = process_object( object_list[i] );
        }

        if( status == VIO_OK )
            print( "Objects processed.\n" );
    }

    if( status == VIO_OK )
        status = output_graphics_file( output_filename, format,
                                       n_objects, object_list );

    if( status == VIO_OK )
        delete_object_list( n_objects, object_list );

    if( status == VIO_OK )
        print( "Objects output.\n" );

    return( status != VIO_OK );
}
