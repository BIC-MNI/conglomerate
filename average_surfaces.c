#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status           status;
    STRING           filename, output_filename;
    int              i, n_objects, n_surfaces;
    Colour           *colours;
    File_formats     format;
    object_struct    *out_object;
    object_struct    **object_list;
    polygons_struct  *polygons, *average_polygons;
    Point            **points_list;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) )
    {
        print_error(
          "Usage: %s output.obj [input1.obj] [input2.obj] ...\n",
          argv[0] );
        return( 1 );
    }

    n_surfaces = 0;

    out_object = create_object( POLYGONS );
    average_polygons = get_polygons_ptr(out_object);

    while( get_string_argument( NULL, &filename ) )
    {
        if( input_graphics_file( filename, &format, &n_objects,
                                 &object_list ) != OK )
        {
            print( "Couldn't read %s.\n", filename );
            return( 1 );
        }

        if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
        {
            print( "Invalid object in file.\n" );
            return( 1 );
        }

        polygons = get_polygons_ptr( object_list[0] );

        if( n_surfaces == 0 )
        {
            copy_polygons( polygons, average_polygons );

            (void) get_object_colours( out_object, &colours );
            REALLOC( colours, polygons->n_points );
            set_object_colours( out_object, colours );
            average_polygons->colour_flag = PER_VERTEX_COLOURS;
        }
        else if( !polygons_are_same_topology( average_polygons, polygons ) )
        {
            print( "Invalid polygons topology in file.\n" );
            return( 1 );
        }

        SET_ARRAY_SIZE( points_list, n_surfaces, n_surfaces+1, 1 );
        ALLOC( points_list[n_surfaces], polygons->n_points );

        for_less( i, 0, polygons->n_points )
            points_list[n_surfaces][i] = polygons->points[i];

        ++n_surfaces;

        print( "%d:  %s\n", n_surfaces, filename );

        delete_object_list( n_objects, object_list );
    }

    compute_polygon_normals( average_polygons );

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       1, &out_object );

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}
