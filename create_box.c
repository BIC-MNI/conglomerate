#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    STRING          output_filename;
    int             ind;
    object_struct   *object;
    polygons_struct *polygons;
    Real            x1, x2, y1, y2, z1, z2;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &x1 ) ||
        !get_real_argument( 0.0, &x2 ) ||
        !get_real_argument( 0.0, &y1 ) ||
        !get_real_argument( 0.0, &y2 ) ||
        !get_real_argument( 0.0, &z1 ) ||
        !get_real_argument( 0.0, &z2 ) )
    {
        print_error( "Usage: %s  filename.obj x1 x2 y1 y2 z1 z2\n", argv[0] );
        return( 1 );
    }

    object = create_object( POLYGONS );
    polygons = get_polygons_ptr(object);

    initialize_polygons( polygons, WHITE, NULL );

    polygons->n_points = 8;
    ALLOC( polygons->points, polygons->n_points );
    ALLOC( polygons->normals, polygons->n_points );

    fill_Point( polygons->points[0], x1, y1, z1 );
    fill_Point( polygons->points[1], x1, y1, z2 );
    fill_Point( polygons->points[2], x1, y2, z1 );
    fill_Point( polygons->points[3], x1, y2, z2 );
    fill_Point( polygons->points[4], x2, y1, z1 );
    fill_Point( polygons->points[5], x2, y1, z2 );
    fill_Point( polygons->points[6], x2, y2, z1 );
    fill_Point( polygons->points[7], x2, y2, z2 );

    polygons->n_items = 6;
    ALLOC( polygons->end_indices, polygons->n_items );

    polygons->end_indices[0] = 4;
    polygons->end_indices[1] = 8;
    polygons->end_indices[2] = 12;
    polygons->end_indices[3] = 16;
    polygons->end_indices[4] = 20;
    polygons->end_indices[5] = 24;

    ALLOC( polygons->indices, polygons->end_indices[polygons->n_items-1] );

    ind = 0;
    polygons->indices[ind++] = 0;
    polygons->indices[ind++] = 1;
    polygons->indices[ind++] = 3;
    polygons->indices[ind++] = 2;

    polygons->indices[ind++] = 4;
    polygons->indices[ind++] = 6;
    polygons->indices[ind++] = 7;
    polygons->indices[ind++] = 5;

    polygons->indices[ind++] = 0;
    polygons->indices[ind++] = 4;
    polygons->indices[ind++] = 5;
    polygons->indices[ind++] = 1;

    polygons->indices[ind++] = 2;
    polygons->indices[ind++] = 3;
    polygons->indices[ind++] = 7;
    polygons->indices[ind++] = 6;

    polygons->indices[ind++] = 0;
    polygons->indices[ind++] = 2;
    polygons->indices[ind++] = 6;
    polygons->indices[ind++] = 4;

    polygons->indices[ind++] = 1;
    polygons->indices[ind++] = 5;
    polygons->indices[ind++] = 7;
    polygons->indices[ind++] = 3;

    compute_polygon_normals( polygons );

    (void) output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );

    return( 0 );
}
