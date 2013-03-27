#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_Status          status;
    int             resolution;
    char            *output_filename;
    object_struct   *object;
    VIO_Real            cx, cy, cz, rx, ry;
    int             axis;
    VIO_Point           centre;

    status = VIO_OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &cx ) ||
        !get_real_argument( 0.0, &cy ) ||
        !get_real_argument( 0.0, &cz ) ||
        !get_int_argument( 0, &axis ) ||
        !get_real_argument( 0.0, &rx ) ||
        !get_real_argument( 0.0, &ry ) ||
        !get_int_argument( 0, &resolution ) )
    {
        (void) fprintf( stderr, "Must have a filename, etc.\n" );
        return( 1 );
    }

    object = create_object( LINES );

    fill_Point( centre, cx, cy, cz );
    create_line_circle( &centre, axis, rx, ry, resolution,
                        get_lines_ptr(object) );

    status = output_graphics_file( output_filename, ASCII_FORMAT, 1, &object );

    if( status == VIO_OK )
        delete_object( object );

    if( status == VIO_OK )
        print( "Lines output.\n" );

    return( status != VIO_OK );
}
