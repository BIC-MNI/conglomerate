#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  [output.obj] [iters] [ratio]\n\
\n\
     Smooths the polygon normals, placing output in input.obj, or if\n\
     specified, output.obj.   It performs iters iterations of replacing\n\
     each normal with the ratio interpolation between itself and the\n\
     average of its neighbours.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    Status               status;
    STRING               src_filename, dest_filename;
    int                  i, n_iters, n_objects;
    Real                 threshold;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void)  get_string_argument( src_filename, &dest_filename );
    (void)  get_int_argument( 5, &n_iters );
    (void)  get_real_argument( 0.9, &threshold );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type( object_list[i] ) == POLYGONS )
        {
            polygons = get_polygons_ptr(object_list[i]);
            average_polygon_normals( polygons, n_iters, threshold );
        }
    }

    status = output_graphics_file( dest_filename, format,
                                   n_objects, object_list );

    delete_object_list( n_objects, object_list );

    return( status != OK );
}
