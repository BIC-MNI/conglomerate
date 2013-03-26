#include  <volume_io.h>
#include  <bicpl.h>

static  void   coalesce_lines(
    lines_struct  *lines );

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input.obj  output.obj\n\
\n\
     Coalesces any shared points in a lines object.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               src_filename, dest_filename;
    VIO_File_formats         format;
    int                  i;
    int                  n_objects;
    object_struct        **objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &dest_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type(objects[i]) == LINES )
            coalesce_lines( get_lines_ptr(objects[i]) );
    }

    if( output_graphics_file( dest_filename, format, n_objects, objects ) != OK)
        return( 1 );

    delete_object_list( n_objects, objects );

    return( 0 );
}

static  void   coalesce_indices(
    lines_struct  *lines )
{
    int            i, j, p, line, size, point_index, neigh_index, n;
    int            p1, p2, n_indices, neighbour, **neighbours, *n_neighbours;

    ALLOC( neighbours, lines->n_points );
    ALLOC( n_neighbours, lines->n_points );

    for_less( i, 0, lines->n_points )
        n_neighbours[i] = 0;

    for_less( line, 0, lines->n_items )
    {
        size = GET_OBJECT_SIZE( *lines, line );

        for_less( i, 0, size-1 )
        {
            p1 = lines->indices[POINT_INDEX( lines->end_indices, line, i )];
            p2 = lines->indices[POINT_INDEX( lines->end_indices, line, i+1 )];

            ADD_ELEMENT_TO_ARRAY( neighbours[p1], n_neighbours[p1], p2, 1 );
            ADD_ELEMENT_TO_ARRAY( neighbours[p2], n_neighbours[p2], p1, 1 );
        }
    }

    FREE( lines->indices );
    FREE( lines->end_indices );
    lines->n_items = 0;
    n_indices = 0;

    for_less( p, 0, lines->n_points )
    {
        for_less( n, 0, n_neighbours[p] )
        {
            if( neighbours[p][n] >= 0 )
            {
                point_index = p;
                neigh_index = n;

                ADD_ELEMENT_TO_ARRAY( lines->indices, n_indices,
                                      point_index, DEFAULT_CHUNK_SIZE );

                do
                {
                    neighbour = neighbours[point_index][neigh_index];

                    for_less( j, 0, n_neighbours[neighbour] )
                    {
                        if( neighbours[neighbour][j] == point_index )
                            break;
                    }

                    if( j == n_neighbours[neighbour] )
                        handle_internal_error(" j == n_neighbours[neighbour]" );

                    neighbours[point_index][neigh_index] = -1;
                    neighbours[neighbour][j] = -1;

                    ADD_ELEMENT_TO_ARRAY( lines->indices, n_indices,
                                          neighbour, DEFAULT_CHUNK_SIZE );

                    for_less( neigh_index, 0, n_neighbours[neighbour] )
                    {
                        if( neighbours[neighbour][neigh_index] >= 0 )
                            break;
                    }

                    if( neigh_index == n_neighbours[point_index] )
                        point_index = -1;
                    else
                        point_index = neighbour;
                }
                while( point_index >= 0 && point_index != p );

                ADD_ELEMENT_TO_ARRAY( lines->end_indices, lines->n_items,
                                      n_indices, DEFAULT_CHUNK_SIZE );
            }
        }
    }

    for_less( p, 0, lines->n_points )
    {
        if( n_neighbours[p] > 0 )
            FREE( neighbours[p] );
    }

    FREE( neighbours );
    FREE( n_neighbours );
}

static  void   coalesce_lines(
    lines_struct  *lines )
{
    int               i, j, n_new_points, *translations;
    int               n_indices;
    VIO_Point             *new_points;

    n_new_points = 0;
    new_points = NULL;

    ALLOC( translations, lines->n_points );

    for_less( i, 0, lines->n_points )
    {
        for_less( j, 0, n_new_points )
        {
            if( EQUAL_POINTS( lines->points[i], new_points[j] ) )
                break;
        }

        if( j != n_new_points )
            translations[i] = j;
        else
        {
            translations[i] = n_new_points;
            ADD_ELEMENT_TO_ARRAY( new_points, n_new_points, lines->points[i],
                                  DEFAULT_CHUNK_SIZE );
        }
    }

    if( n_new_points < lines->n_points )
    {
        n_indices = NUMBER_INDICES(*lines);
        for_less( i, 0, n_indices )
            lines->indices[i] = translations[lines->indices[i]];
        FREE( lines->points );
        lines->n_points = n_new_points;
        lines->points = new_points;
    }

    coalesce_indices( lines );

    FREE( translations );
}
