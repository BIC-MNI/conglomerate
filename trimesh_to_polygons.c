#include <volume_io/internal_volume_io.h>
#include <special_geometry.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.msh  output.obj  output.mid\n\
\n\
     Converts the trimesh to a polgyons file.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING             output_mid_filename, input_mesh_filename;
    STRING             output_filename;
    object_struct      *object;
    tri_mesh_struct    mesh;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_mesh_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &output_mid_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( tri_mesh_input( input_mesh_filename, BINARY_FORMAT, &mesh ) != OK )
        return( 1 );

    object = create_object( POLYGONS );

    tri_mesh_convert_to_polygons( &mesh, get_polygons_ptr(object) );

    set_use_compressed_polygons_flag( FALSE );
    (void) output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );

    output_mesh_fixed_midpoints( output_mid_filename, &mesh );

    return( 0 );
}
