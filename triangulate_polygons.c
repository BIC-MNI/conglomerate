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

private  BOOLEAN  recursive_triangulate_one_polygon(
    int     size,
    int     poly[],
    int     n_neighbours[],
    int     *neighbours[],
    int     indices[] )
{
    int       *left, *right, n_left, n_right, p1, p2;
    int       start_index, end_index, count, i, n;
    BOOLEAN   found;

    if( size < 3 )
        handle_internal_error( "recursive_triangulate_one_polygon" );
    
    if( size == 3 )
    {
        indices[0] = poly[0];
        indices[1] = poly[1];
        indices[2] = poly[2];
        return( TRUE );
    }

    found = FALSE;

    ALLOC( left, size );
    ALLOC( right, size );

    for_less( start_index, 0, size-2 )
    {
        p1 = poly[start_index];

        for_less( end_index, start_index+2, size )
        {
            if( start_index == 0 && end_index == size-1 )
                continue;

            p2 = poly[end_index];

            count = 0;
            for_less( n, 0, n_neighbours[p1] )
            {
                if( neighbours[p1][n] == p2 )
                    ++count;
            }

            if( count != 1 )
                continue;

            n_left = 0;
            for_inclusive( i, 0, start_index )
                left[n_left++] = poly[i];
            for_less( i, end_index, size )
                left[n_left++] = poly[i];

            n_right = 0;
            for_inclusive( i, start_index, end_index )
                right[n_right++] = poly[i];

            if( n_left + n_right != size + 2 )
            {
                handle_internal_error( "n_left" );
            }

            if( recursive_triangulate_one_polygon( n_left, left,
                                       n_neighbours, neighbours, indices ) &&
                recursive_triangulate_one_polygon( n_right, right,
                                       n_neighbours, neighbours,
                                       &indices[3*(n_left-2)] ) )
            {
                found = TRUE;
                break;
            }
        }

        if( found )
            break;
    }

    FREE( left );
    FREE( right );

    return( found );
}

private  BOOLEAN  triangulate_one_polygon(
    int     size,
    int     poly[],
    int     n_neighbours[],
    int     *neighbours[],
    int     indices[] )
{
    return( recursive_triangulate_one_polygon( size, poly,
                                n_neighbours, neighbours, indices ) );
}

private  void  triangulate_polygons(
    polygons_struct  *polygons,
    polygons_struct  *triangles )
{
    int                poly, size, index, ind, n_matches, n;
    int                *n_neighbours, **neighbours, *indices, max_size;
    progress_struct    progress;
    BOOLEAN            done;

    create_polygon_point_neighbours( polygons, TRUE,
                                     &n_neighbours, &neighbours,
                                     NULL, NULL );

    *triangles = *polygons;

    triangles->colour_flag = ONE_COLOUR;
    ALLOC( triangles->colours, 1 );
    triangles->colours[0] = polygons->colours[0];

    triangles->points = polygons->points;
    triangles->normals = polygons->normals;

    triangles->n_items = 0;
    for_less( poly, 0, polygons->n_items )
        triangles->n_items += GET_OBJECT_SIZE( *polygons, poly ) - 2;

    ALLOC( triangles->indices, 3 * triangles->n_items );

    max_size = 0;
    for_less( poly, 0, polygons->n_items )
        max_size = MAX( max_size, GET_OBJECT_SIZE( *polygons, poly ) );

    ALLOC( indices, max_size );

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Triangulating" );

    ind = 0;
    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        for_less( index, 0, size )
        {
            indices[index] = polygons->indices[POINT_INDEX(
                                            polygons->end_indices,poly,index)];
        }

        for_less( index, 2, size-1 )
        {
            n_matches = 0;
            for_less( n, 0, n_neighbours[indices[0]] )
            {
                if( neighbours[indices[0]][n] == indices[index] )
                    ++n_matches;
            }

            if( n_matches != 1 )
                break;
        }

        if( size > 3 && index < size-1 )
        {
            done = triangulate_one_polygon( size, indices,
                                              n_neighbours, neighbours,
                                              &triangles->indices[ind] );
            if( !done )
                print( "Could not find good triangulation: %d\n", poly );
            else
                ind += 3 * (size-2);
        }
        else
            done = FALSE;

        if( !done )
        {
            for_less( index, 1, size-1 )
            {
                triangles->indices[ind] = indices[0];
                ++ind;
                triangles->indices[ind] = indices[index];
                ++ind;
                triangles->indices[ind] = indices[index+1];
                ++ind;
            }
        }

        update_progress_report( &progress, poly+1 );
    }

    terminate_progress_report( &progress );

    if( ind != 3 * triangles->n_items )
        handle_internal_error( "Summation of ind" );

    FREE( polygons->end_indices );
    FREE( polygons->indices );
    FREE( indices );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     NULL, NULL );

    ALLOC( triangles->end_indices, triangles->n_items );
    for_less( poly, 0, triangles->n_items )
        triangles->end_indices[poly] = 3 * (poly + 1);
}
