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
    int    poly, p, size, index, n_indices;

    *triangles = *polygons;

    triangles->colour_flag = ONE_COLOUR;
    ALLOC( triangles->colours, 1 );
    triangles->colours[0] = polygons->colours[0];

    ALLOC( triangles->points, triangles->n_points );
    for_less( p, 0, triangles->n_points )
        triangles->points[p] = polygons->points[p];

    if( polygons->normals != NULL )
    {
        ALLOC( triangles->normals, triangles->n_points );
        for_less( p, 0, triangles->n_points )
            triangles->normals[p] = polygons->normals[p];
    }

    triangles->n_items = 0;
    n_indices = 0;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( index, 1, size-1 )
        {
            ADD_ELEMENT_TO_ARRAY( triangles->end_indices,
                                  triangles->n_items, n_indices + 3,
                                  DEFAULT_CHUNK_SIZE );
            ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices,
                  polygons->indices[POINT_INDEX(polygons->end_indices,poly,0)],
                                  DEFAULT_CHUNK_SIZE );
            ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices,
               polygons->indices[POINT_INDEX(polygons->end_indices,poly,index)],
                                  DEFAULT_CHUNK_SIZE );
            ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices,
             polygons->indices[POINT_INDEX(polygons->end_indices,poly,index+1)],
                                  DEFAULT_CHUNK_SIZE );
        }
    }
}
