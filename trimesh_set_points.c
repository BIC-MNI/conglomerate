#include <volume_io.h>
#include <special_geometry.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  dest.msh|obj  src.mesh|obj  output.obj\n\
\n\
     Sets the trimesh points to the given points.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING             dest_mesh_filename, src_mesh_filename;
    STRING             output_filename;
    object_struct      *object;
    tri_mesh_struct    src_mesh, dest_mesh;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &dest_mesh_filename ) ||
        !get_string_argument( NULL, &src_mesh_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( tri_mesh_input( dest_mesh_filename, BINARY_FORMAT, &dest_mesh ) != OK )
        return( 1 );

    if( tri_mesh_input( src_mesh_filename, BINARY_FORMAT, &src_mesh ) != OK )
        return( 1 );

    tri_mesh_reconcile_points( &dest_mesh, &src_mesh );

    object = create_object( POLYGONS );

    tri_mesh_convert_to_polygons( &dest_mesh, get_polygons_ptr(object) );

    set_use_compressed_polygons_flag( FALSE );
    (void) output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );

    return( 0 );
}
