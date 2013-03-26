#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *output_file, *variance_file;
    VIO_STR           avg_filename, sample_filename, output_filename;
    VIO_STR           variance_filename;
    int              p, i, j, n_objects, n_avg_objects, n_points, n_avg_points;
    VIO_File_formats     format;
    object_struct    **object_list, **avg_object_list;
    VIO_Point            *points, *avg_points;
    VIO_Vector           offset;
    VIO_Real             transform[3][3], tx, ty, tz, mahalanobis;

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

        tx = (VIO_Real) Vector_x(offset) * transform[0][0] +
             (VIO_Real) Vector_y(offset) * transform[1][0] +
             (VIO_Real) Vector_z(offset) * transform[2][0];
        ty = (VIO_Real) Vector_x(offset) * transform[0][1] +
             (VIO_Real) Vector_y(offset) * transform[1][1] +
             (VIO_Real) Vector_z(offset) * transform[2][1];
        tz = (VIO_Real) Vector_x(offset) * transform[0][2] +
             (VIO_Real) Vector_y(offset) * transform[1][2] +
             (VIO_Real) Vector_z(offset) * transform[2][2];

        mahalanobis = tx * (VIO_Real) Vector_x(offset) +
                      ty * (VIO_Real) Vector_y(offset) +
                      tz * (VIO_Real) Vector_z(offset);

        if( output_real( output_file, mahalanobis ) != OK ||
            output_newline( output_file ) != OK )
            return( 1 );
    }

    (void) close_file( variance_file );
    (void) close_file( output_file );

    return( 0 );
}
