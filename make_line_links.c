#include  <volume_io.h>
#include  <bicpl.h>

#define  TOLERANCE  0.0

static  void  tmp_coalesce_object_points(
    int      *n_points,
    VIO_Point    *points[],
    int      n_indices,
    int      indices[] );

static  void  coalesce_lines(
    lines_struct   *lines );

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input_lines1.obj  input_lines2.obj output_lines.obj n_links\n\
\n\
     Creates a set of line segments linking the two input lines.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               src1_filename, src2_filename, dest_filename;
    VIO_File_formats         format;
    int                  n_objects1, n_objects2, n_links, i, obj_index;
    object_struct        **objects1, **objects2, *object;
    lines_struct         *lines1, *lines;
    VIO_Point                point1, point2;
    VIO_Real                 length;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src1_filename ) ||
        !get_string_argument( NULL, &src2_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_int_argument( 0, &n_links ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src1_filename,
                             &format, &n_objects1, &objects1 ) != OK ||
        n_objects1 != 1 || get_object_type(objects1[0]) != LINES )
    {
        print( "Expected one lines object in file: %s\n", src1_filename );
        return( 1 );
    }

    lines1 = get_lines_ptr( objects1[0] );

    tmp_coalesce_object_points( &lines1->n_points, &lines1->points,
                            lines1->end_indices[lines1->n_items-1],
                            lines1->indices );

    coalesce_lines( lines1 );

    if( output_graphics_file( dest_filename, format, 1, objects1 ) != OK )
        return( 1 );

    if( lines1->n_items != 1 )
    {
        print( "Expected one lines item in file: %s (%d,%d)\n", src1_filename,
                lines1->n_items, lines1->n_points );
        return( 1 );
    }

return( 0 );

    if( input_graphics_file( src2_filename,
                             &format, &n_objects2, &objects2 ) != OK ||
        n_objects2 != 1 || get_object_type(objects2[0]) != LINES )
    {
        print( "Expected one lines object in file: %s\n", src2_filename );
        return( 1 );
    }

    object = create_object( LINES );
    lines = get_lines_ptr( object );
    initialize_lines( lines, WHITE );

    length = get_lines_length( lines1 );
    for_less( i, 0, n_links )
    {
        get_lines_arc_point( lines1, (VIO_Real) i / (VIO_Real) n_links * length,
                             &point1 );

        (void) find_closest_point_on_object( &point1, objects2[0],
                                             &obj_index, &point2 );

        start_new_line( lines );
        add_point_to_line( lines, &point1 );
        add_point_to_line( lines, &point2 );
    }

    if( output_graphics_file( dest_filename, format, 1, &object ) != OK )
        return( 1 );

    delete_object_list( n_objects1, objects1 );

    return( 0 );
}


static  void  coalesce_lines(
    lines_struct   *lines )
{
    int   p, n, l, *n_neighbours, **neighbours, total_neighbours;
    int   size, p1, p2, n_items, n_indices, *indices, *end_indices, edge;

    if( lines->n_points <= 0 ||
        lines->n_items <= 0 )
    {
        return;
    }

    ALLOC( n_neighbours, lines->n_points  );

    for_less( p, 0, lines->n_points )
        n_neighbours[p] = 0;

    for_less( l, 0, lines->n_items )
    {
        size = GET_OBJECT_SIZE( *lines, l );

        for_less( edge, 0, size-1 )
        {
            p1 = lines->indices[POINT_INDEX(lines->end_indices,l,edge)];
            p2 = lines->indices[POINT_INDEX(lines->end_indices,l,edge+1)];

            ++n_neighbours[p1];
            ++n_neighbours[p2];
        }
    }

    print( "\n" );

    total_neighbours = 0;
    for_less( p, 0, lines->n_points )
    {
        if( n_neighbours[p] != 0 && n_neighbours[p] != 2 )
        {
            return;
        }
        total_neighbours += n_neighbours[p];
    }

    ALLOC( neighbours, lines->n_points );
    ALLOC( neighbours[0], total_neighbours );
    for_less( p, 1, lines->n_points )
    {
        neighbours[p] = &neighbours[p-1][n_neighbours[p-1]];
    }

    for_less( p, 0, lines->n_points )
        n_neighbours[p] = 0;

    for_less( l, 0, lines->n_items )
    {
        size = GET_OBJECT_SIZE( *lines, l );

        for_less( edge, 0, size-1 )
        {
            p1 = lines->indices[POINT_INDEX(lines->end_indices,l,edge)];
            p2 = lines->indices[POINT_INDEX(lines->end_indices,l,edge+1)];

            neighbours[p1][n_neighbours[p1]] = p2;
            ++n_neighbours[p1];
            neighbours[p2][n_neighbours[p2]] = p1;
            ++n_neighbours[p2];
        }
    }

    n_items = 0;
    end_indices = NULL;
    n_indices = 0;
    indices = NULL;

    for_less( l, 0, lines->n_items )
    {
        size = GET_OBJECT_SIZE( *lines, l );

        for_less( edge, 0, size-1 )
        {
            p1 = lines->indices[POINT_INDEX(lines->end_indices,l,edge)];
            p2 = lines->indices[POINT_INDEX(lines->end_indices,l,edge+1)];

            for_less( n, 0, n_neighbours[p1] )
            {
                if( neighbours[p1][n] == p2 )
                    break;
            }

            if( n >= n_neighbours[p1] )
                continue;

            ADD_ELEMENT_TO_ARRAY( indices, n_indices, p1, DEFAULT_CHUNK_SIZE );

            while( n < n_neighbours[p1] )
            {
                ADD_ELEMENT_TO_ARRAY( indices, n_indices, p2,
                                      DEFAULT_CHUNK_SIZE );

                neighbours[p1][n] = -1;

                for_less( n, 0, n_neighbours[p2] )
                {
                    if( neighbours[p2][n] == p1 )
                        break;
                }

                if( n >= n_neighbours[p1] )
                {
                    handle_internal_error( "coalesce_lines" );
                    return;
                }

                neighbours[p2][n] = -1;

                p1 = p2;

                for_less( n, 0, n_neighbours[p1] )
                {
                    if( neighbours[p1][n] >= 0 )
                        break;
                }

                p2 = neighbours[p1][n];
            }

            ADD_ELEMENT_TO_ARRAY( end_indices, n_items, n_indices,
                                  DEFAULT_CHUNK_SIZE );
        }
    }

    FREE( lines->end_indices );
    FREE( lines->indices );

    lines->n_items = n_items;
    lines->indices = indices;
    lines->end_indices = end_indices;
}

#define  N_BOX_RATIO   0.05
#define  MIN_N_BOXES   10

typedef  struct
{
    int   n_points;
    int   *point_indices;
} box_struct;

static  void  get_box_index(
    VIO_Point   *point,
    VIO_Point   *min_point,
    VIO_Point   *max_point,
    int     n_boxes[],
    int     *i,
    int     *j,
    int     *k )
{
    VIO_Real  diff, p, min_p, max_p;
    int   dim, *coords[VIO_N_DIMENSIONS];

    coords[X] = i;
    coords[Y] = j;
    coords[Z] = k;

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        p = (VIO_Real) Point_coord(*point,dim);
        min_p = (VIO_Real) Point_coord(*min_point,dim);
        max_p = (VIO_Real) Point_coord(*max_point,dim);
        if( p <= min_p )
            *(coords[dim]) = 0;
        else if( p >= max_p )
            *(coords[dim]) = n_boxes[dim]-1;
        else
        {
            diff = max_p - min_p;
            if( diff <= 0.0 )
                *(coords[dim]) = 0;
            else
                *(coords[dim]) = (int) ((VIO_Real) n_boxes[dim] *
                                        (p - min_p) / diff);
        }
    } 
}

static  void  tmp_coalesce_object_points(
    int      *n_points,
    VIO_Point    *points[],
    int      n_indices,
    int      indices[] )
{
    int         n_boxes[VIO_N_DIMENSIONS];
    int         i, j, k, p, point, ind, *points_list, *translation;
    int         cum_index, dim, new_n_points;
    int         i1, i2, j1, j2, k1, k2;
    box_struct  ***boxes;
    VIO_Point       min_point, max_point, *new_points, pt;
    BOOLEAN     found;

    new_points = NULL;

    get_range_points( *n_points, *points, &min_point, &max_point );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        n_boxes[dim] = (int) pow( (VIO_Real) *n_points * N_BOX_RATIO, 0.3333 );
        if( n_boxes[dim] < MIN_N_BOXES )
            n_boxes[dim] = MIN_N_BOXES;
        if( Point_coord(min_point,dim) == Point_coord(max_point,dim) )
            n_boxes[dim] = 1;
    }

    VIO_ALLOC3D( boxes, n_boxes[X], n_boxes[Y], n_boxes[Z] );

    for_less( i, 0, n_boxes[X] )
    for_less( j, 0, n_boxes[Y] )
    for_less( k, 0, n_boxes[Z] )
    {
        boxes[i][j][k].n_points = 0;
    }

    for_less( point, 0, *n_points )
    {
        get_box_index( &(*points)[point], &min_point, &max_point, n_boxes,
                       &i, &j, &k );
        ++boxes[i][j][k].n_points;
    }

    ALLOC( points_list, *n_points );

    cum_index = 0;
    for_less( i, 0, n_boxes[X] )
    for_less( j, 0, n_boxes[Y] )
    for_less( k, 0, n_boxes[Z] )
    {
        boxes[i][j][k].point_indices = &points_list[cum_index];
        cum_index += boxes[i][j][k].n_points;
        boxes[i][j][k].n_points = 0;
    }

    for_less( point, 0, *n_points )
    {
        get_box_index( &(*points)[point], &min_point, &max_point, n_boxes,
                       &i, &j, &k );
        boxes[i][j][k].point_indices[boxes[i][j][k].n_points] = point;
        ++boxes[i][j][k].n_points;
    }

    ALLOC( translation, *n_points );
    for_less( point, 0, *n_points )
        translation[point] = -1;

    new_n_points = 0;

    for_less( point, 0, *n_points )
    {
        fill_Point( pt, RPoint_x((*points)[point]) - TOLERANCE,
                       RPoint_y((*points)[point]) - TOLERANCE,
                       RPoint_z((*points)[point]) - TOLERANCE );
        get_box_index( &pt, &min_point, &max_point, n_boxes, &i1, &j1, &k1 );

        fill_Point( pt, RPoint_x((*points)[point]) + TOLERANCE,
                       RPoint_y((*points)[point]) + TOLERANCE,
                       RPoint_z((*points)[point]) + TOLERANCE );
        get_box_index( &pt, &min_point, &max_point, n_boxes, &i2, &j2, &k2 );

        found = FALSE;
        for_inclusive( i, i1, i2 )
        for_inclusive( j, j1, j2 )
        for_inclusive( k, k1, k2 )
        {
            if( found )
                break;

            for_less( p, 0, boxes[i][j][k].n_points )
            {
                ind = boxes[i][j][k].point_indices[p];
                if( ind < point && distance_between_points( &(*points)[point],
                                &(*points)[ind] ) <= TOLERANCE * TOLERANCE )
                {
                    found = TRUE;
                    break;
                }
            }
        }

        if( found )
            translation[point] = translation[ind];
        else
        {
            translation[point] = new_n_points;
            ADD_ELEMENT_TO_ARRAY( new_points, new_n_points,
                                  (*points)[point],
                                  DEFAULT_CHUNK_SIZE );
        }
    }

    for_less( i, 0, n_indices )
        indices[i] = translation[indices[i]];

    VIO_FREE3D( boxes );
    FREE( points_list );
    FREE( translation );

    FREE( *points );
    *n_points = new_n_points;
    *points = new_points;
}
