#include  <def_mni.h>
#include  <def_module.h>

private  void  output_points(
    FILE             *file,
    polygons_struct  *polygons );

int  main(
    int  argc,
    char *argv[] )
{
    Status         status;
    char           *filename;
    FILE           *input_file;
    File_formats   format;
    object_struct  *object;
    Boolean        eof;

    initialize_argument_processing( argc, argv );

    status = OK;

    if( get_string_argument( "", &filename ) )
    {
        status = open_file( filename, READ_FILE, BINARY_FORMAT, &input_file );
    }
    else
        input_file = stdin;

    if( status == OK )
        status = input_object( "", input_file, &format, &object, &eof );

    if( status == OK && object->object_type == POLYGONS )
        output_points( stdout, get_polygons_ptr(object) );

    if( input_file != stdin )
        status = close_file( input_file );

    return( status != OK );
}

private  void  output_points(
    FILE             *file,
    polygons_struct  *polygons )
{
    Point   *points;
    int     tess, n_up, n_around, around, up;

    if( !get_tessellation_of_polygons_sphere(polygons,&tess) )
    {
        print( "error, not sphere topology.\n" );
        return;
    }

    n_up = tess;
    n_around = 2 * tess;
    points = polygons->points;

    (void) output_int( file, n_up );
    (void) output_int( file, n_around );
    (void) output_newline( file );

    (void) io_point( file, WRITE_FILE, ASCII_FORMAT,
              &points[get_sphere_point_index( 0, 0, n_up, n_around )] );
    (void) output_newline( file );
    (void) output_newline( file );

    for_less( up, 1, n_up )
    {
        for_less( around, 0, n_around )
        {
            (void) io_point( file, WRITE_FILE, ASCII_FORMAT,
               &points[get_sphere_point_index( up, around, n_up, n_around )] );
            (void) output_newline( file );
        }
        (void) output_newline( file );
    }

    io_point( file, WRITE_FILE, ASCII_FORMAT,
              &points[get_sphere_point_index( n_up, 0, n_up, n_around )] );
    (void) output_newline( file );
}
