#include  <module.h>

private  void   convert_polygons_to_lines(
    polygons_struct   *polygons,
    lines_struct      *lines );

#define  BINTREE_FACTOR  0.5

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *src_polygons_filename, *dest_lines_filename;
    File_formats         format;
    int                  i;
    int                  n_objects, n_dest_objects;
    object_struct        **objects, **dest_objects, *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_polygons_filename ) ||
        !get_string_argument( "", &dest_lines_filename ) )
    {
        print( "Usage: %s  src_polygons  dest_lines\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_polygons_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    n_dest_objects = 0;

    for_less( i, 0, n_objects )
    {
        if( get_object_type( objects[i] ) == POLYGONS )
        {
            object = create_object( LINES );
            convert_polygons_to_lines( get_polygons_ptr(objects[i]),
                                       get_lines_ptr(object) );

            add_object_to_list( &n_dest_objects, &dest_objects, object );
        }
    }

    if( output_graphics_file( dest_lines_filename, format, n_dest_objects,
                              dest_objects ) != OK )
        return( 1 );

    delete_object_list( n_objects, objects );
    delete_object_list( n_dest_objects, dest_objects );

    return( 0 );
}

private  void   convert_polygons_to_lines(
    polygons_struct   *polygons,
    lines_struct      *lines )
{
    int   poly, edge, size, point_index, index;
    int   point_index1, point_index2;

    check_polygons_neighbours_computed( polygons );

    initialize_lines( lines, WHITE );

    lines->n_points = polygons->n_points;
    ALLOC( lines->points, lines->n_points );

    for_less( point_index, 0, lines->n_points )
        lines->points[point_index] = polygons->points[point_index];

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( edge, 0, size )
        {
            index = POINT_INDEX( polygons->end_indices, poly, edge );
            if( polygons->neighbours[index] < 0 ||
                polygons->neighbours[index] > poly )
            {
                start_new_line( lines );

                point_index1 = polygons->indices[index];
                point_index2 = polygons->indices[
                  POINT_INDEX(polygons->end_indices,poly,(edge+1)%size)];

                ADD_ELEMENT_TO_ARRAY( lines->indices,
                                      lines->end_indices[lines->n_items-1],
                                      point_index1, DEFAULT_CHUNK_SIZE );
                ADD_ELEMENT_TO_ARRAY( lines->indices,
                                      lines->end_indices[lines->n_items-1],
                                      point_index2, DEFAULT_CHUNK_SIZE );
            }
        }
    }
}
