#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input1_filename, input2_filename, output_filename;
    int              i, n_objects;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons1, *polygons2;
    Point            point;
    Real             dist, min_dist, max_dist, rms, avg_dist, std_dev;
    Real             sum_x, sum_xx;
    BOOLEAN          outputting;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) )
    {
        print_error(
          "Usage: %s  input1.obj  input2.obj [output.txt]\n", argv[0] );
        return( 1 );
    }

    outputting = get_string_argument( NULL, &output_filename );

    if( input_graphics_file( input1_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input1_filename );
        return( 1 );
    }

    polygons1 = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( input2_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input2_filename );
        return( 1 );
    }

    polygons2 = get_polygons_ptr( object_list[0] );

    create_polygons_bintree( polygons2,
                             ROUND( (Real) polygons2->n_items *
                                    BINTREE_FACTOR ) );

    if( outputting &&
        open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    sum_x = 0.0;
    sum_xx = 0.0;
    min_dist = 0.0;
    max_dist = 0.0;

    for_less( i, 0, polygons1->n_points )
    {
        (void) find_closest_polygon_point( &polygons1->points[i], polygons2,
                                           &point );
        dist = distance_between_points( &polygons1->points[i], &point );

        if( i == 0 || dist < min_dist )
            min_dist = dist;
        if( i == 0 || dist > max_dist )
            max_dist = dist;

        sum_x += dist;
        sum_xx += dist * dist;

        if( outputting )
        {
            if( output_real( file, dist ) != OK ||
                output_newline( file ) != OK )
                return( 1 );
        }
    }

    min_dist = sqrt( min_dist );
    max_dist = sqrt( max_dist );
    rms = sqrt( sum_xx / (Real) polygons1->n_points );

    avg_dist = sum_x / (Real) polygons1->n_points;

    std_dev = (sum_xx - sum_x * sum_x / (Real) polygons1->n_points) /
              (Real) (polygons1->n_points-1);

    if( std_dev > 0.0 )
        std_dev = sqrt( std_dev );

    print( "Rms over the %d points: %g\n", polygons1->n_points, rms );
    print( "Average           dist: %g\n", avg_dist );
    print( "Std dev           dist: %g\n", std_dev );
    print( "Min               dist: %g\n", min_dist );
    print( "Max               dist: %g\n", max_dist );

    if( outputting )
        (void) close_file( file );

    return( 0 );
}
