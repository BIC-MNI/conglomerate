#include  <volume_io.h>
#include  <bicpl.h>

private  void  build_polygons(
    polygons_struct  *polygons );

int  main(
    int   argc,
    char  *argv[] )
{
    char             *output_name;
    int              n_objects;
    object_struct    *object, **object_list;
    polygons_struct  *polygons;
    Surfprop         spr;

    initialize_argument_processing( argc, argv );

    (void) get_string_argument( "out.obj", &output_name );

    n_objects = 0;
    object = create_object( POLYGONS );
    add_object_to_list( &n_objects, &object_list, object );

    polygons = get_polygons_ptr( object );

    fill_Surfprop( spr, 0.6, 0.3, 0.3, 5.0, 1.0 );

    initialize_polygons( polygons, make_Colour_0_1( 0.6, 0.6, 0.5 ), &spr );

    build_polygons( polygons );
    
    (void) output_graphics_file( output_name, ASCII_FORMAT,
                                 n_objects, object_list );

    delete_object_list( n_objects, object_list );

    return( 0 );
}

#define  FRONT_WIDTH    50.0
#define  FRONT_HEIGHT   40.0

#define  MIDDLE_WIDTH1    51.0
#define  MIDDLE_HEIGHT1   41.0

#define  DEPTH1           10.0
#define  DEPTH2            0.1
#define  DEPTH3            5.0
#define  DEPTH4           10.0
#define  DEPTH5           25.0

#define  MIDDLE_WIDTH2    50.0
#define  MIDDLE_HEIGHT2   40.0

#define  MIDDLE_WIDTH3    50.0
#define  MIDDLE_HEIGHT3   40.0

#define  MIDDLE_WIDTH5    30.0
#define  MIDDLE_HEIGHT5   20.0

private  void  add_point(
    polygons_struct  *polygons,
    VIO_Real             x,
    VIO_Real             y,
    VIO_Real             z,
    VIO_Real             nx,
    VIO_Real             ny,
    VIO_Real             nz )
{
    VIO_Point   point;
    VIO_Vector  normal;

    fill_Point( point, x, y, z );
    fill_Vector( normal, nx, ny, nz );
    NORMALIZE_VECTOR( normal, normal );

    add_point_to_polygon( polygons, &point, &normal );
}

private  void  build_polygons(
    polygons_struct  *polygons )
{
    VIO_Real  depth;

    start_new_polygon( polygons );

    add_point( polygons, -FRONT_WIDTH / 2.0, -FRONT_HEIGHT / 2.0, 0.0,
                         1.0, 0.0, 0.0 );
    add_point( polygons, -FRONT_WIDTH / 2.0,  FRONT_HEIGHT / 2.0, 0.0,
                         1.0, 0.0, 0.0 );
    add_point( polygons, -MIDDLE_WIDTH1 / 2.0,  MIDDLE_HEIGHT1 / 2.0, DEPTH1,
                         1.0, 0.0, 0.0 );
    add_point( polygons, -MIDDLE_WIDTH1 / 2.0, -MIDDLE_HEIGHT1 / 2.0, DEPTH1,
                         1.0, 0.0, 0.0 );

    start_new_polygon( polygons );

    add_point( polygons, FRONT_WIDTH / 2.0, -FRONT_HEIGHT / 2.0, 0.0,
                         -1.0, 0.0, 0.0 );
    add_point( polygons, MIDDLE_WIDTH1 / 2.0,  -MIDDLE_HEIGHT1 / 2.0, DEPTH1,
                         -1.0, 0.0, 0.0 );
    add_point( polygons, MIDDLE_WIDTH1 / 2.0,  MIDDLE_HEIGHT1 / 2.0, DEPTH1,
                         -1.0, 0.0, 0.0 );
    add_point( polygons, FRONT_WIDTH / 2.0, FRONT_HEIGHT / 2.0, 0.0,
                         -1.0, 0.0, 0.0 );

    start_new_polygon( polygons );

    add_point( polygons, FRONT_WIDTH / 2.0, -FRONT_HEIGHT / 2.0, 0.0,
                         0.0, 1.0, 0.0 );
    add_point( polygons, -FRONT_WIDTH / 2.0, -FRONT_HEIGHT / 2.0, 0.0,
                         0.0, 1.0, 0.0 );
    add_point( polygons, -MIDDLE_WIDTH1 / 2.0,  -MIDDLE_HEIGHT1 / 2.0, DEPTH1,
                         0.0, 1.0, 0.0 );
    add_point( polygons, MIDDLE_WIDTH1 / 2.0,  -MIDDLE_HEIGHT1 / 2.0, DEPTH1,
                         0.0, 1.0, 0.0 );

    start_new_polygon( polygons );

    add_point( polygons, -FRONT_WIDTH / 2.0, FRONT_HEIGHT / 2.0, 0.0,
                         0.0, -1.0, 0.0 );
    add_point( polygons, FRONT_WIDTH / 2.0, FRONT_HEIGHT / 2.0, 0.0,
                         0.0, -1.0, 0.0 );
    add_point( polygons, MIDDLE_WIDTH1 / 2.0,  MIDDLE_HEIGHT1 / 2.0, DEPTH1,
                         0.0, -1.0, 0.0 );
    add_point( polygons, -MIDDLE_WIDTH1 / 2.0,  MIDDLE_HEIGHT1 / 2.0, DEPTH1,
                         0.0, -1.0, 0.0 );

    depth = DEPTH1 + DEPTH2 + DEPTH3 + DEPTH4 + DEPTH5;

    start_new_polygon( polygons );

    add_point( polygons, -FRONT_WIDTH / 2.0, -FRONT_HEIGHT / 2.0, 0.0,
                         1.0, 0.0, 0.0 );
    add_point( polygons, -FRONT_WIDTH / 2.0,  FRONT_HEIGHT / 2.0, 0.0,
                         1.0, 0.0, 0.0 );
    add_point( polygons, -MIDDLE_WIDTH5 / 2.0,  MIDDLE_HEIGHT5 / 2.0, depth,
                         1.0, 0.0, 0.0 );
    add_point( polygons, -MIDDLE_WIDTH5 / 2.0, -MIDDLE_HEIGHT5 / 2.0, depth,
                         1.0, 0.0, 0.0 );

    start_new_polygon( polygons );

    add_point( polygons, FRONT_WIDTH / 2.0, -FRONT_HEIGHT / 2.0, 0.0,
                         -1.0, 0.0, 0.0 );
    add_point( polygons, MIDDLE_WIDTH5 / 2.0,  -MIDDLE_HEIGHT5 / 2.0, depth,
                         -1.0, 0.0, 0.0 );
    add_point( polygons, MIDDLE_WIDTH5 / 2.0,  MIDDLE_HEIGHT5 / 2.0, depth,
                         -1.0, 0.0, 0.0 );
    add_point( polygons, FRONT_WIDTH / 2.0, FRONT_HEIGHT / 2.0, 0.0,
                         -1.0, 0.0, 0.0 );

    start_new_polygon( polygons );

    add_point( polygons, FRONT_WIDTH / 2.0, -FRONT_HEIGHT / 2.0, 0.0,
                         0.0, 1.0, 0.0 );
    add_point( polygons, -FRONT_WIDTH / 2.0, -FRONT_HEIGHT / 2.0, 0.0,
                         0.0, 1.0, 0.0 );
    add_point( polygons, -MIDDLE_WIDTH5 / 2.0,  -MIDDLE_HEIGHT5 / 2.0, depth,
                         0.0, 1.0, 0.0 );
    add_point( polygons, MIDDLE_WIDTH5 / 2.0,  -MIDDLE_HEIGHT5 / 2.0, depth,
                         0.0, 1.0, 0.0 );

    start_new_polygon( polygons );

    add_point( polygons, -FRONT_WIDTH / 2.0, FRONT_HEIGHT / 2.0, 0.0,
                         0.0, -1.0, 0.0 );
    add_point( polygons, FRONT_WIDTH / 2.0, FRONT_HEIGHT / 2.0, 0.0,
                         0.0, -1.0, 0.0 );
    add_point( polygons, MIDDLE_WIDTH5 / 2.0,  MIDDLE_HEIGHT5 / 2.0, depth,
                         0.0, -1.0, 0.0 );
    add_point( polygons, -MIDDLE_WIDTH5 / 2.0,  MIDDLE_HEIGHT5 / 2.0, depth,
                         0.0, -1.0, 0.0 );
}
