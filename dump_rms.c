#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input1_filename, input2_filename, output_filename;
    int              i, n_objects, n_points1, n_points2;
    Real             dx, dy, dz, dist_sq, rms, dist;
    Real             sum_x, sum_xx, std_dev, avg_dist, min_dist, max_dist;
    File_formats     format;
    object_struct    **object_list;
    Point            *points1, *points2;
    BOOLEAN          output_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) )
    {
        print_error(
          "Usage: %s input1.obj  input2.obj  output.txt\n",
                  argv[0] );
        return( 1 );
    }

    output_flag = get_string_argument( NULL, &output_filename );

    if( input_graphics_file( input1_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 )
    {
        print( "Couldn't read %s.\n", input1_filename );
        return( 1 );
    }

    n_points1 = get_object_points( object_list[0], &points1 );

    if( input_graphics_file( input2_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 )
    {
        print( "Couldn't read %s.\n", input2_filename );
        return( 1 );
    }

    n_points2 = get_object_points( object_list[0], &points2 );

    if( n_points1 != n_points2 )
    {
        print( "Number of points not equal.\n" );
        return( 1 );
    }

    if( output_flag &&
        open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    sum_x = 0.0;
    sum_xx = 0.0;
    min_dist = 0.0;
    max_dist = 0.0;

    for_less( i, 0, n_points1 )
    {
        dx = (Real) Point_x(points1[i]) - (Real) Point_x(points2[i]);
        dy = (Real) Point_y(points1[i]) - (Real) Point_y(points2[i]);
        dz = (Real) Point_z(points1[i]) - (Real) Point_z(points2[i]);

        dist_sq = dx * dx + dy * dy + dz * dz;

        if( i == 0 || dist_sq < min_dist )
            min_dist = dist_sq;
        if( i == 0 || dist_sq > max_dist )
            max_dist = dist_sq;

        if( output_flag )
        {
            if( dist_sq <= 0.0 )
                dist = 0.0;
            else
                dist = sqrt( dist_sq );

            (void) output_real( file, dist );
            (void) output_newline( file );
        }

        if( dist_sq > 0.0 )
        {
            sum_x += sqrt( dist_sq );
            sum_xx += dist_sq;
        }
    }

    if( output_flag )
        (void) close_file( file );

    if( min_dist > 0.0 )
        min_dist = sqrt( min_dist );

    if( max_dist > 0.0 )
        max_dist = sqrt( max_dist );

    rms = sqrt( sum_xx / (Real) n_points1 );
    avg_dist = sum_x / (Real) n_points1;
    std_dev = (sum_xx - sum_x * sum_x / (Real) n_points1) /
              (Real) (n_points1-1);
    if( std_dev > 0.0 )
        std_dev = sqrt( std_dev );

    print( "Rms over the %d points: %g\n", n_points1, rms );
    print( "Average           dist: %g\n", avg_dist );
    print( "Std dev           dist: %g\n", std_dev );
    print( "Min               dist: %g\n", min_dist );
    print( "Max               dist: %g\n", max_dist );

    return( 0 );
}
