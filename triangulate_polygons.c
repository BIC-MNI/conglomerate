#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  triangulate_polygons(
    polygons_struct  *polygons,
    polygons_struct  *triangles );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename;
    int                  n_objects;
    File_formats         format;
    object_struct        **object_list, *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &dest_filename ) )
    {
        (void) fprintf( stderr, "Must have two filename arguments.\n" );
        return( 1 );
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    object = create_object( POLYGONS );

    triangulate_polygons( get_polygons_ptr(object_list[0]),
                          get_polygons_ptr(object) );

    (void) output_graphics_file( dest_filename, format, 1, &object );

    return( 0 );
}

private  void  triangulate_polygons(
    polygons_struct  *polygons,
    polygons_struct  *triangles )
{
    int                poly, size, index, n_indices;
    progress_struct    progress;

    *triangles = *polygons;

    triangles->colour_flag = ONE_COLOUR;
    ALLOC( triangles->colours, 1 );
    triangles->colours[0] = polygons->colours[0];

    triangles->points = polygons->points;
    triangles->normals = polygons->normals;

    triangles->n_items = 0;
    n_indices = 0;

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Triangulating" );

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( index, 1, size-1 )
        {
            ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices,
                                  polygons->indices[POINT_INDEX(
                                    polygons->end_indices,poly,0)],
                                  DEFAULT_CHUNK_SIZE );

            ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices,
                                  polygons->indices[POINT_INDEX(
                                    polygons->end_indices,poly,index)],
                                  DEFAULT_CHUNK_SIZE );

            ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices,
                                  polygons->indices[POINT_INDEX(
                                        polygons->end_indices,poly,index+1)],
                                  DEFAULT_CHUNK_SIZE );

            ++triangles->n_items;
        }

        update_progress_report( &progress, poly+1 );
    }

    terminate_progress_report( &progress );

    FREE( polygons->end_indices );
    FREE( polygons->indices );

    ALLOC( triangles->end_indices, triangles->n_items );
    for_less( poly, 0, triangles->n_items )
        triangles->end_indices[poly] = 3 * (poly + 1);
}
