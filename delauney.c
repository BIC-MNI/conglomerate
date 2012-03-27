#include  <volume_io.h>
#include  <bicpl.h>

private  void  create_delauney(
    int               n_points,
    Real              (*points)[2],
    polygons_struct   *triangles );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING          input_filename, output_filename;
    File_formats    format;
    int             n_objects;
    int             i, p, n_points, n_object_points;
    Point           *object_points;
    Real            (*points)[2];
    object_struct   **objects, *object;
    polygons_struct *triangles;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: \n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects, &objects )
                             != OK )
        return( 1 );

    n_points = 0;
    points = NULL;

    for_less( i, 0, n_objects )
    {
        n_object_points = get_object_points( objects[i], &object_points );
        SET_ARRAY_SIZE( points, n_points, n_points + n_object_points,
                        DEFAULT_CHUNK_SIZE );
        for_less( p, 0, n_object_points )
        {
            points[n_points+p][X] = RPoint_x( object_points[p] );
            points[n_points+p][Y] = RPoint_y( object_points[p] );
        }

        n_points += n_object_points;
    }

    object = create_object( POLYGONS );
    triangles = get_polygons_ptr( object );

    create_delauney( n_points, points, triangles );

    (void) output_graphics_file( output_filename, format, 1, &object );

    return( 0 );
}

private  void  find_closest_pair(
    int               n_points,
    Real              (*points)[2],
    int               *p1,
    int               *p2 )
{
    Real  closest, dist, dx, dy;
    int   i, j;

    closest = -1.0;
    for_less( i, 0, n_points-1 )
    {
        for_less( j, i+1, n_points )
        {
            dx = points[i][0] - points[j][0];
            dy = points[i][1] - points[j][1];
            dist = dx * dx + dy * dy;
            if( closest < 0.0 || dist < closest )
            {
                closest = dist;
                *p1 = i;
                *p2 = j;
            }
        }
    }
}

private  BOOLEAN  is_clockwise(
    Real              (*points)[2],
    int               p1,
    int               p2,
    int               p3 )
{
    Real  cross;

    cross = (points[p2][0] - points[p1][0]) *
            (points[p3][1] - points[p1][1]) -
            (points[p2][1] - points[p1][1]) *
            (points[p3][0] - points[p1][0]);

    return( cross > 0.0 );
}

private  BOOLEAN  get_circle_through_points(
    Real    (*points)[2],
    int     p1,
    int     p2,
    int     p3,
    Real    *cx,
    Real    *cy,
    Real    *radius2 )
{
    Real   x1, y1, x2, y2, x3, y3, bottom, r2_a_b, c1, c2, c3;

    x1 = points[p1][0];
    y1 = points[p1][1];
    x2 = points[p2][0];
    y2 = points[p2][1];
    x3 = points[p3][0];
    y3 = points[p3][1];

    bottom = (y2*x3-y2*x1-y1*x3-y3*x2+y3*x1+y1*x2);

    if( bottom == 0.0 )
        return( FALSE );

    c1 = x1 * x1 + y1 * y1;
    c2 = x2 * x2 + y2 * y2;
    c3 = x3 * x3 + y3 * y3;

    *cx = -2.0 * -(y2*c3-y2*c1-y1*c3-y3*c2+y3*c1+y1*c2) / bottom;
    *cy = -2.0 * (x2*c3-x1*c3+x1*c2-c2*x3+c1*x3-c1*x2) / bottom;
    r2_a_b = (-c1*y2*x3+x2*y3*c1+x1*y2*c3-x1*y3*c2-x2*y1*c3+c2*y1*x3) / bottom;

    *radius2 = *cx * *cx + *cy * *cy - r2_a_b;

    return( *radius2 > 0.0 );
}

private  BOOLEAN  get_third_point(
    int               n_points,
    Real              (*points)[2],
    int               p1,
    int               p2,
    int               *p3 )
{
    int       p;
    Real      smallest_radius, cx, cy, radius2;
    BOOLEAN   found, clockwise;

    smallest_radius = -1.0;
    found = FALSE;

    for_less( p, 0, n_points )
    {
        if( p == p1 || p == p2 )
            continue;

        clockwise = is_clockwise( points, p1, p2, p );
        if( !clockwise )
            continue;

        get_circle_through_points( points, p1, p2, p, &cx, &cy, &radius2 );

        if( !found || radius2 < smallest_radius )
        {
            found = TRUE;
            smallest_radius = radius2;
            *p3 = p;
        }
    }

    return( found );
}

private  void  create_delauney(
    int               n_points,
    Real              (*points)[2],
    polygons_struct   *triangles )
{
    int                            p, p1, p2, p3, n_indices, tri[3];
    int                            next_p1, next_p2, edge, other;
    Smallest_int                   **done_flags;
    unsigned long                  entry;
    QUEUE_STRUCT( unsigned long )  queue;

    initialize_polygons( triangles, WHITE, NULL );
    triangles->n_points = n_points;
    ALLOC( triangles->points, n_points );
    ALLOC( triangles->normals, n_points );
    for_less( p, 0, n_points )
    {
        fill_Point( triangles->points[p], points[p][0], points[p][1], 0.0 );
        fill_Vector( triangles->normals[p], 0.0, 0.0, 1.0 );
    }
    n_indices = 0;

    ALLOC2D( done_flags, n_points, n_points );
    for_less( p1, 0, n_points )
    for_less( p2, 0, n_points )
        done_flags[p1][p2] = FALSE;

    find_closest_pair( n_points, points, &p1, &p2 );

    print( "Start: %d %d\n", p1, p2 );

    INITIALIZE_QUEUE( queue );

    entry = IJ( (unsigned long) p1, (unsigned long) p2,
                (unsigned long) n_points );
    INSERT_IN_QUEUE( queue, entry );
    entry = IJ( (unsigned long) p2, (unsigned long) p1,
                (unsigned long) n_points );
    INSERT_IN_QUEUE( queue, entry );

    while( !IS_QUEUE_EMPTY(queue) )
    {
        REMOVE_FROM_QUEUE( queue, entry );

        p2 = (int) (entry % (unsigned long) n_points);
        entry /= (unsigned long) n_points;
        p1 = (int) entry;

        if( done_flags[p1][p2] )
            continue;

        if( !get_third_point( n_points, points, p1, p2, &p3 ) )
            continue;

        print( "%d %d %d\n", p1, p2, p3 );

        tri[0] = p1;
        tri[1] = p2;
        tri[2] = p3;

        done_flags[p1][p2] = TRUE;
        done_flags[p2][p3] = TRUE;
        done_flags[p3][p1] = TRUE;

        ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices, tri[0], 100 );
        ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices, tri[1], 100 );
        ADD_ELEMENT_TO_ARRAY( triangles->indices, n_indices, tri[2], 100 );
        ADD_ELEMENT_TO_ARRAY( triangles->end_indices, triangles->n_items,
                              n_indices, 100 );

        for_less( edge, 1, 3 )
        {
            next_p2 = tri[edge];
            next_p1 = tri[(edge+1)%3];

            if( !done_flags[next_p1][next_p2] )
            {
                entry = IJ( (unsigned long) next_p1, (unsigned long) next_p2,
                            (unsigned long) n_points );
                INSERT_IN_QUEUE( queue, entry );
            }
        }
    }

    DELETE_QUEUE( queue );
    FREE3D( done_flags );
}
