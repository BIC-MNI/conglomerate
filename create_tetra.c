#include  <mni.h>
#include  <module.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status          status;
    char            *output_filename;
    int             n_triangles;
    Point           centre;
    object_struct   *object;
    polygons_struct *polygons;
    Real            cx, cy, cz, rx, ry, rz;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &cx ) ||
        !get_real_argument( 0.0, &cy ) ||
        !get_real_argument( 0.0, &cz ) ||
        !get_real_argument( 0.0, &rx ) ||
        !get_real_argument( 0.0, &ry ) ||
        !get_real_argument( 0.0, &rz ) )
    {
        (void) fprintf( stderr, "Must have a filename and n_triangles.\n" );
        return( 1 );
    }

    (void) get_int_argument( 4, &n_triangles );

    fill_Point( centre, cx, cy, cz );

    object = create_object( POLYGONS );
    polygons = get_polygons_ptr(object);
    create_tetrahedral_sphere( &centre, rx, ry, rz, n_triangles, polygons );
    compute_polygon_normals( polygons );

    status = output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );

    if( status == OK )
        delete_object( object );

    if( status == OK )
        print( "Tetrahedron output.\n" );

    return( status != OK );
}
