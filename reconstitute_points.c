#include  <internal_volume_io.h>
#include  <bicpl.h>

public  Status  process_object(
    object_struct  *object );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  full_topology.obj  input.pt  output.obj\n\
\n\
     Creates an output file which has the topology of the first argument.\n\
     but the points of the second argument.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING              input_filename, output_filename, points_filename;
    int                 i, pt, n_objects, n_points;
    Point               *points;
    File_formats        format;
    object_struct       **object_list;
    FILE                *file;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &points_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }


    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( open_file( points_filename, READ_FILE, BINARY_FORMAT, &file ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( object_list[i], &points );

        for_less( pt, 0, n_points )
        {
            if( io_point( file, READ_FILE, BINARY_FORMAT, &points[pt] ) != OK )
            {
                print( "Error reading %d'th point.\n", pt );
                return( 1 );
            }
        }

        if( get_object_type( object_list[i] ) == POLYGONS )
            compute_polygon_normals( get_polygons_ptr(object_list[i]) );
        else if( get_object_type( object_list[i] ) == QUADMESH )
            compute_quadmesh_normals( get_quadmesh_ptr(object_list[i]) );
    }

    close_file( file );

    (void) output_graphics_file( output_filename, format,
                                 n_objects, object_list );

    delete_object_list( n_objects, object_list );

    return( 0 );
}
