#include  <bicpl.h>

#define  X_FACTOR  5.0
#define  Y_FACTOR  20.0

private  void    find_plane(
    Point      points[],
    Vector     *normal );
private  void  create_plane_from_points(
    Point             points[],
    polygons_struct   *polygons );

int  main(
    int   argc,
    char  *argv[] )
{
    BOOLEAN              create_plane;
    char                 *three_tags_filename, *input_tags_filename;
    char                 *output_tags_filename, *plane_filename;
    Real                 min_dist, max_dist, dist;
    int                  i;
    Point                points[3], origin, point;
    Vector               normal;
    object_struct        *object;
    int                  n_volumes, n_tag_points, *structure_ids, *patient_ids;
    Real                 **tags1, **tags2, *weights;
    char                 **labels;
    int                  n_new_tags, *new_structure_ids, *new_patient_ids;
    Real                 **new_tags1, **new_tags2, *new_weights;
    char                 **new_labels;


    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &three_tags_filename ) ||
        !get_string_argument( "", &input_tags_filename ) ||
        !get_string_argument( "", &output_tags_filename ) ||
        !get_real_argument( 0.0, &min_dist ) ||
        !get_real_argument( 0.0, &max_dist ) )
    {
        print( "Usage:  %s  3_tags_file  input.tag  output.tag  -distance +distance [plane_filename]\n", argv[0] );
        return( 1 );
    }

    create_plane = get_string_argument( "", &plane_filename );

    if( input_tag_file( three_tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, &weights, &structure_ids,
                        &patient_ids, &labels ) != OK )
        return( 1 );

    if( n_tag_points != 3 )
    {
        print( "Usage:  %s  3_tags_file  input.tag  output.tag\n", argv[0] );
        print( "First file must contain exactly 3 tags.\n" );
        return( 1 );
    }

    for_less( i, 0, 3 )
        fill_Point( points[i], tags1[i][X], tags1[i][Y], tags1[i][Z] );

    free_tag_points( n_volumes, n_tag_points, tags1, tags2,
                     weights, structure_ids, patient_ids, labels );

    find_plane( points, &normal );
    NORMALIZE_VECTOR( normal, normal );
    origin = points[0];

    if( input_tag_file( input_tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, &weights, &structure_ids,
                        &patient_ids, &labels ) != OK )
        return( 1 );

    n_new_tags = 0;

    for_less( i, 0, n_tag_points )
    {
        fill_Point( point, tags1[i][X], tags1[i][Y], tags1[i][Z] );

        dist = DOT_POINT_VECTOR( point, normal ) -
               DOT_POINT_VECTOR( origin, normal );

        if( dist >= min_dist && dist <= max_dist )
        {
            /*--- increase the memory allocation of the tag points */

            SET_ARRAY_SIZE( new_tags1, n_new_tags, n_new_tags+1, 10 );
            ALLOC( new_tags1[n_new_tags], 3 );

            SET_ARRAY_SIZE( new_weights, n_new_tags, n_new_tags+1, 10 );
            SET_ARRAY_SIZE( new_structure_ids, n_new_tags, n_new_tags+1, 10 );
            SET_ARRAY_SIZE( new_patient_ids, n_new_tags, n_new_tags+1, 10 );

            SET_ARRAY_SIZE( new_labels, n_new_tags, n_new_tags+1, 10 );
            ALLOC( new_labels[n_new_tags], strlen(labels[i])+1 );

            /*--- copy from the input tags to the new tags */

            new_tags1[n_new_tags][0] = tags1[i][0];
            new_tags1[n_new_tags][1] = tags1[i][1];
            new_tags1[n_new_tags][2] = tags1[i][2];

            if( n_volumes == 2 )
            {
                SET_ARRAY_SIZE( new_tags2, n_new_tags, n_new_tags+1, 10 );
                ALLOC( new_tags2[n_new_tags], 3 );

                new_tags2[n_new_tags][0] = tags2[i][0];
                new_tags2[n_new_tags][1] = tags2[i][1];
                new_tags2[n_new_tags][2] = tags2[i][2];
            }

            new_weights[n_new_tags] = weights[i];
            new_structure_ids[n_new_tags] = structure_ids[i];
            new_patient_ids[n_new_tags] = patient_ids[i];
            (void) strcpy( new_labels[n_new_tags], labels[i] );

            /*--- increment the number of new tags */

            ++n_new_tags;
        }
    }

    /*--- output the new tags, the subset of the input tags */

    if( output_tag_file( output_tags_filename, "Removed negative X's",
                         n_volumes, n_new_tags, new_tags1, new_tags2,
                         new_weights, new_structure_ids,
                         new_patient_ids, new_labels ) != OK )
        return( 1 );

    free_tag_points( n_volumes, n_new_tags, new_tags1, new_tags2,
                     new_weights, new_structure_ids, new_patient_ids,
                     new_labels );

    if( create_plane )
    {
        object = create_object( POLYGONS );

        create_plane_from_points( points, get_polygons_ptr(object) );

        (void) output_graphics_file( plane_filename, ASCII_FORMAT,
                                     1, &object );
    }

    return( 0 );
}

private  void    find_plane(
    Point      points[],
    Vector     *normal )
{
    Vector   v1, v2;

    SUB_POINTS( v1, points[1], points[0] );
    SUB_POINTS( v2, points[2], points[0] );
    CROSS_VECTORS( *normal, v1, v2 );
}

private  void  get_xy_pos(
    Vector  x_axis,
    Vector  y_axis,
    Real    x,
    Real    y,
    Point   *point )
{
    Vector   x_offset, y_offset, v;

    SCALE_VECTOR( x_offset, x_axis, x );
    SCALE_VECTOR( y_offset, y_axis, y );
    ADD_VECTORS( v, x_offset, y_offset );
    CONVERT_VECTOR_TO_POINT( *point, v );
}

private  void  create_plane_from_points(
    Point             points[],
    polygons_struct   *polygons )
{
    int      i;
    Vector   v1, v2, v3;
    Real     x_min, x_max, y_min, y_max, x, y, dx, dy;
    Point    point;

    SUB_POINTS( v1, points[1], points[0] );
    SUB_POINTS( v2, points[2], points[0] );
    CROSS_VECTORS( v3, v1, v2 );
    CROSS_VECTORS( v2, v3, v1 );

    NORMALIZE_VECTOR( v1, v1 );
    NORMALIZE_VECTOR( v2, v2 );
    NORMALIZE_VECTOR( v3, v3 );

    x_min = 0.0;
    x_max = 0.0;
    y_min = 0.0;
    y_max = 0.0;

    for_less( i, 0, 3 )
    {
        x = DOT_VECTORS( v1, points[i] );
        y = DOT_VECTORS( v2, points[i] );

        if( i == 0 || x < x_min )
            x_min = x;
        if( i == 0 || x > x_max )
            x_max = x;
        if( i == 0 || y < y_min )
            y_min = y;
        if( i == 0 || y > y_max )
            y_max = y;
    }

    initialize_polygons( polygons, WHITE, NULL );

    polygons->n_points = 4;
    ALLOC( polygons->points, polygons->n_points );
    ALLOC( polygons->normals, polygons->n_points );

    polygons->n_items = 1;
    ALLOC( polygons->end_indices, polygons->n_items );
    polygons->end_indices[0] = 4;
    ALLOC( polygons->indices, 4 );
    polygons->indices[0] = 0;
    polygons->indices[1] = 1;
    polygons->indices[2] = 2;
    polygons->indices[3] = 3;

    dx = x_max - x_min;
    dy = y_max - y_min;

    get_xy_pos( v1, v2, x_min - X_FACTOR * dx, y_min - Y_FACTOR * dy, &point );
    polygons->points[0] = point;
    polygons->normals[0] = v3;

    get_xy_pos( v1, v2, x_max + X_FACTOR * dx, y_min - Y_FACTOR * dy, &point );
    polygons->points[1] = point;
    polygons->normals[1] = v3;

    get_xy_pos( v1, v2, x_max + X_FACTOR * dx, y_max + Y_FACTOR * dy, &point );
    polygons->points[2] = point;
    polygons->normals[2] = v3;

    get_xy_pos( v1, v2, x_min - X_FACTOR * dx, y_max + Y_FACTOR * dy, &point );
    polygons->points[3] = point;
    polygons->normals[3] = v3;
}
