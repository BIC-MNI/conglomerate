#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  [output.obj colour_name]\n\
\n\
     Copies the input objects to the output objects, with the colour set\n\
     accordingly.  If only 1 arg specified, simply prints colour of object\n\n";

    print_error( usage_str, executable );
}


int  main(
    int   argc,
    char  *argv[] )
{
    STRING               src_filename, dest_filename, colour_name;
    File_formats         format;
    int                  i;
    Colour               colour, *colours;
    int                  n_objects;
    object_struct        **objects;
    BOOLEAN              setting_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    setting_flag = get_string_argument( NULL, &dest_filename ) &&
                   get_string_argument( NULL, &colour_name );

    if( input_graphics_file( src_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );


    if( setting_flag )
    {
        colour = convert_string_to_colour( colour_name ); 
        for_less( i, 0, n_objects )
            set_object_colour( objects[i], colour );

        if( output_graphics_file( dest_filename, format,
                                  n_objects, objects ) != OK)
            return( 1 );
    }
    else
    {
        for_less( i, 0, n_objects )
        {
            (void) get_object_colours( objects[i], &colours );
            colour_name = convert_colour_to_string( colours[0] );
            print( "%s\n", colour_name );
            delete_string( colour_name );
        }
    }

    delete_object_list( n_objects, objects );

    return( 0 );
}
