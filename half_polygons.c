#include <internal_volume_io.h>
#include <bicpl.h>


private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  [output.obj]\n\
\n\
     Subdivides any polygons in the file, placing output in the original file\n\
     or in a different output file.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING         input_filename, output_filename;
    int            i, n_objects;
    File_formats   format;
    object_struct  **object_list;
    polygons_struct  *polygons, half;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type(object_list[i]) == POLYGONS )
        {
            polygons = get_polygons_ptr( object_list[i] );

            if( is_this_sphere_topology( polygons ) )
                half_sample_sphere_tessellation( polygons, &half );
            else if( is_this_tetrahedral_topology( polygons ) )
                half_sample_tetrahedral_tessellation( polygons, &half );

            compute_polygon_normals( &half );

            delete_polygons( polygons );
            *polygons = half;
        }
    }

    (void) output_graphics_file( output_filename, format,
                                 n_objects, object_list );

    delete_object_list( n_objects, object_list );

    return( 0 );
}
