#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *output_file, *variance_file;
    STRING           avg_filename, sample_filename, output_filename;
    STRING           variance_filename;
    int              p, i, j, n_objects, n_avg_objects, n_points, n_avg_points;
    File_formats     format;
    object_struct    **object_list, **avg_object_list;
    Point            *points, *avg_points;
    Vector           offset;
    Real             transform[3][3], tx, ty, tz, mahalanobis;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &avg_filename ) ||
        !get_string_argument( NULL, &variance_filename ) ||
        !get_string_argument( NULL, &sample_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
          "Usage: %s  avg.obj variance.txt sample.obj output.txt\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( avg_filename, &format, &n_avg_objects,
                             &avg_object_list ) != OK )
    {
        print( "Couldn't read %s.\n", avg_filename );
        return( 1 );
    }

    if( n_avg_objects != 1 )
    {
        print( "Invalid object in file.\n" );
        return( 1 );
    }

    n_avg_points = get_object_points( avg_object_list[0], &avg_points );

    if( input_graphics_file( sample_filename, &format, &n_objects,
                             &object_list ) != OK )
    {
        print( "Couldn't read %s.\n", sample_filename );
        return( 1 );
    }

    if( n_objects != 1 )
    {
        print( "Invalid object in file.\n" );
        return( 1 );
    }

    n_points = get_object_points( object_list[0], &points );

    if( n_points != n_avg_points )
    {
        print( "Mismatch in size.\n" );
        return( 1 );
    }

    if( open_file( variance_filename, READ_FILE, ASCII_FORMAT, &variance_file )
        != OK )
        return( 1 );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &output_file )
        != OK )
        return( 1 );

    for_less( p, 0, n_points )
    {
        SUB_POINTS( offset, points[p], avg_points[p] );

        for_less( i, 0, 3 )
        for_less( j, 0, 3 )
            if( input_real( variance_file, &transform[i][j] ) != OK )
                return( 1 );

        tx = (Real) Vector_x(offset) * transform[0][0] +
             (Real) Vector_y(offset) * transform[1][0] +
             (Real) Vector_z(offset) * transform[2][0];
        ty = (Real) Vector_x(offset) * transform[0][1] +
             (Real) Vector_y(offset) * transform[1][1] +
             (Real) Vector_z(offset) * transform[2][1];
        tz = (Real) Vector_x(offset) * transform[0][2] +
             (Real) Vector_y(offset) * transform[1][2] +
             (Real) Vector_z(offset) * transform[2][2];

        mahalanobis = tx * (Real) Vector_x(offset) +
                      ty * (Real) Vector_y(offset) +
                      tz * (Real) Vector_z(offset);

        if( output_real( output_file, mahalanobis ) != OK ||
            output_newline( output_file ) != OK )
            return( 1 );
    }

    (void) close_file( variance_file );
    (void) close_file( output_file );

    return( 0 );
}
