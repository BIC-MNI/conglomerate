#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status          status;
    int             resolution;
    char            *output_filename;
    object_struct   *object;
    Real            cx, cy, cz, rx, ry, rz;
    Point           centre;
    Surfprop        spr;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) ||
        !get_int_argument( 0, &resolution ) )
    {
        (void) fprintf( stderr, "Must have a filename and resolution.\n" );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &cx );
    (void) get_real_argument( 0.0, &cy );
    (void) get_real_argument( 0.0, &cz );
    (void) get_real_argument( 1.0, &rx );
    (void) get_real_argument( 1.0, &ry );
    (void) get_real_argument( 1.0, &rz );

    object = create_object( POLYGONS );

    fill_Surfprop( spr, 0.4, 0.5, 0.5, 40.0, 1.0 );
    initialize_polygons( get_polygons_ptr(object), WHITE, &spr );

    fill_Point( centre, cx, cy, cz );
    create_polygons_sphere( &centre, rx, ry, rz, resolution, 2 * resolution,
                            FALSE, get_polygons_ptr(object) );

    status = output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );

    if( status == OK )
        delete_object( object );

    if( status == OK )
        print( "Sphere output.\n" );

    return( status != OK );
}
