#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               output_filename;
    Real                 limits[2][3];
    int                  a1, a2, axis;
    object_struct        *object;
    polygons_struct      *polygons;
    Point                points[4];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &limits[0][X] ) ||
        !get_real_argument( 0.0, &limits[1][X] ) ||
        !get_real_argument( 0.0, &limits[0][Y] ) ||
        !get_real_argument( 0.0, &limits[1][Y] ) ||
        !get_real_argument( 0.0, &limits[0][Z] ) ||
        !get_real_argument( 0.0, &limits[1][Z] ) )
    {
        print_error( "Usage: %s  output.obj  x1 x2 y1 y2 z1 z2\n", argv[0] );
        print_error( "\n" );
        print_error( "           creates a rectangle.\n" );
        return( 1 );
    }

    if( limits[0][X] == limits[1][X] )
        axis = X;
    else if( limits[0][Y] == limits[1][Y] )
        axis = Y;
    else if( limits[0][Z] == limits[1][Z] )
        axis = Z;

    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    object = create_object( POLYGONS );
    polygons = get_polygons_ptr(object);
    initialize_polygons( polygons, WHITE, NULL );

    fill_Point( points[0], limits[0][X], limits[0][Y], limits[0][Z] );
    fill_Point( points[1], limits[0][X], limits[0][Y], limits[0][Z] );
    fill_Point( points[2], limits[0][X], limits[0][Y], limits[0][Z] );
    fill_Point( points[3], limits[0][X], limits[0][Y], limits[0][Z] );

    Point_coord( points[1], a1 ) = (Point_coord_type) limits[1][a1];
    Point_coord( points[2], a1 ) = (Point_coord_type) limits[1][a1];
    Point_coord( points[2], a2 ) = (Point_coord_type) limits[1][a2];
    Point_coord( points[3], a2 ) = (Point_coord_type) limits[1][a2];

    add_point_to_polygon( polygons, &points[0], NULL );
    add_point_to_polygon( polygons, &points[1], NULL );
    add_point_to_polygon( polygons, &points[2], NULL );
    add_point_to_polygon( polygons, &points[3], NULL );

    ALLOC( polygons->normals, 4 );

    compute_polygon_normals( polygons );

    if( output_graphics_file( output_filename, ASCII_FORMAT, 1, &object ) != OK)
        return( 1 );

    return( 0 );
}
