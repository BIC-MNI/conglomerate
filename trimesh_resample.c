#include <volume_io.h>
#include <special_geometry.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj|input.msh  input.obj output.msh output.obj output.mid\n\
         subdivide_values min_value max_value min_size max_size\n\
         coalesce_values min_value max_value min_size max_size\n\
\n\
     Subdivides and coalesces the triangular mesh.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING             input_mesh_filename, input_obj_filename;
    STRING             output_mid_filename, output_obj_filename;
    STRING             output_mesh_filename;
    STRING             input_sub_filename, input_coal_filename;
    int                n_objects, new_n_polys, n_values;
    int                original_n_points;
    File_formats       format;
    object_struct      **object_list, *object;
    polygons_struct    *polygons;
    Real               *values;
    Real               min_sub_value, max_sub_value, min_sub_size, max_sub_size;
    Real               min_coal_value, max_coal_value;
    Real               min_coal_size, max_coal_size;
    tri_mesh_struct    mesh;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_mesh_filename ) ||
        !get_string_argument( NULL, &input_obj_filename ) ||
        !get_string_argument( NULL, &output_mesh_filename ) ||
        !get_string_argument( NULL, &output_obj_filename ) ||
        !get_string_argument( NULL, &output_mid_filename ) ||
        !get_string_argument( NULL, &input_sub_filename ) ||
        !get_real_argument( 0.0, &min_sub_value ) ||
        !get_real_argument( 0.0, &max_sub_value ) ||
        !get_real_argument( 0.0, &min_sub_size ) ||
        !get_real_argument( 0.0, &max_sub_size ) ||
        !get_string_argument( NULL, &input_coal_filename ) ||
        !get_real_argument( 0.0, &min_coal_value ) ||
        !get_real_argument( 0.0, &max_coal_value ) ||
        !get_real_argument( 0.0, &min_coal_size ) ||
        !get_real_argument( 0.0, &max_coal_size ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( tri_mesh_input( input_mesh_filename, BINARY_FORMAT, &mesh ) != OK )
        return( 1 );

    if( input_graphics_file( input_obj_filename, &format, &n_objects,
                             &object_list ) != OK ||
        n_objects < 1 || get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error in input file.\n" );
        return( 1 );
    }

    if( !tri_mesh_set_points( &mesh,
                              get_polygons_ptr(object_list[0])->n_points,
                              get_polygons_ptr(object_list[0])->points ) )
        return( 1 );

    delete_object_list( n_objects, object_list );

    original_n_points = tri_mesh_get_n_points( &mesh );

    if( !equal_strings( input_sub_filename, "none" ) )
    {
        if( input_texture_values( input_sub_filename, &n_values, &values ) != OK
            || n_values != original_n_points )
        {
            print_error( "Error inputting subdivide node values.\n" );
            return( 1 );
        }

        tri_mesh_subdivide_triangles( &mesh, min_sub_value, max_sub_value,
                                      n_values, values,
                                      min_sub_size, max_sub_size, -1 );

        FREE( values );
    }

    if( !equal_strings( input_coal_filename, "none" ) )
    {
        if( input_texture_values( input_coal_filename, &n_values,
                                  &values ) != OK ||
            n_values != original_n_points )
        {
            print_error( "Error inputting coalesce node values.\n" );
            return( 1 );
        }

        tri_mesh_coalesce_triangles( &mesh, min_coal_value, max_coal_value,
                                     n_values, values,
                                     min_coal_size, max_coal_size );

        FREE( values );
    }

    tri_mesh_subdivide_bordering_triangles( &mesh );

    tri_mesh_delete_unused_nodes( &mesh );

    (void) tri_mesh_output( output_mesh_filename, BINARY_FORMAT, &mesh );

    object = create_object( POLYGONS );
    polygons = get_polygons_ptr( object );
    tri_mesh_convert_to_polygons( &mesh, polygons );

    set_use_compressed_polygons_flag( FALSE );
    (void) output_graphics_file( output_obj_filename, format, 1, &object );

    (void) output_mesh_fixed_midpoints( output_mid_filename, &mesh );

    new_n_polys = polygons->n_items;

    print( "Resampled into %d polygons.\n", new_n_polys );

    tri_mesh_print_levels( &mesh );

    return( 0 );
}
