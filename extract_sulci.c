#include  <bicpl.h>

#define  INVALID_INDEX    -1

private  void  extract_sulci(
    polygons_struct   *polygons,
    VIO_Real              curvature_threshold,
    lines_struct      *lines )
{
    int               i, ind, n_sulcal_points, point_index, poly, vertex;
    int               *indices, size;
    VIO_BOOL           *points_done;
    VIO_Real              *curvatures, base_length;
    VIO_Point             centroid;
    VIO_Vector            normal;
    progress_struct   progress;

    check_polygons_neighbours_computed( polygons );

    initialize_lines( lines, WHITE );

    if( polygons->n_points == 0 )
        return;

    ALLOC( curvatures, polygons->n_points );
    ALLOC( indices, polygons->n_points );

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Calculating Curvature" );

    ALLOC( points_done, polygons->n_points );
    for_less( i, 0, polygons->n_points )
        points_done[i] = FALSE;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( vertex, 0, size )
        {
            point_index = polygons->indices[
                         POINT_INDEX( polygons->end_indices, poly, vertex)];
            if( !points_done[point_index] )
            {
                points_done[point_index] = TRUE;

                compute_polygon_point_centroid( polygons, poly, vertex,
                                 point_index, &centroid, &normal,
                                 &base_length, &curvatures[point_index] );

            }
        }

        update_progress_report( &progress, poly );
    }

    terminate_progress_report( &progress );

    FREE( points_done );

    n_sulcal_points = 0;

    for_less( i, 0, polygons->n_points )
    {
        if( curvatures[i] < -curvature_threshold )
        {
            indices[i] = n_sulcal_points;
            ++n_sulcal_points;
        }
        else
            indices[i] = INVALID_INDEX;
    }

    if( n_sulcal_points == 0 )
    {
        FREE( indices );
        FREE( curvatures );
        return;
    }

    lines->n_points = n_sulcal_points;
    ALLOC( lines->points, n_sulcal_points );

    ind = 0;
    for_less( i, 0, polygons->n_points )
    {
        if( indices[i] != INVALID_INDEX )
        {
            lines->points[ind] = polygons->points[i];
            ++ind;
        }
    }

    lines->n_items = n_sulcal_points;
    ALLOC( lines->end_indices, lines->n_items );
    ALLOC( lines->indices, lines->n_items );
    for_less( i, 0, lines->n_points )
    {
        lines->end_indices[i] = i + 1;
        lines->indices[i] = i;
    }

    FREE( indices );
    FREE( curvatures );
}

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    File_formats         format;
    int                  n_objects;
    VIO_Real                 curvature_threshold;
    object_struct        **objects, *lines;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  input_surface  output_surface]\n",
               argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.2, &curvature_threshold );

    if( input_graphics_file( input_filename,
                             &format, &n_objects, &objects ) != VIO_OK )
        return( 1 );

    if( n_objects != 1 || get_object_type( objects[0] ) != POLYGONS )
    {
        print( "File must contain exactly 1 set of polygons.\n" );
        return( 1 );
    }

    lines = create_object( LINES );
    extract_sulci( get_polygons_ptr(objects[0]), curvature_threshold,
                   get_lines_ptr( lines ) );

    if( output_graphics_file( output_filename, BINARY_FORMAT, 1, &lines ) != VIO_OK)
        return( 1 );

    delete_object_list( n_objects, objects );
    delete_object( lines );

    return( 0 );
}
