#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.4

typedef enum { LINKED_DISTANCE, NEAREST_DISTANCE, NORMAL_DISTANCE }
             Distance_methods;

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input1_filename, input2_filename, output_filename;
    STRING           method_name;
    Distance_methods method;
    Vector           normal;
    int              i, n_objects, obj_index, n_intersections_not_found;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons1, *polygons2;
    Point            point;
    Real             dist, min_dist, max_dist, rms, avg_dist, std_dev;
    Real             sum_x, sum_xx, bintree_factor;
    BOOLEAN          outputting;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) ||
        !get_string_argument( NULL, &method_name ) )
    {
        print_error(
          "Usage: %s  input1.obj  input2.obj link|near|normal [output.txt]\n", argv[0] );
        return( 1 );
    }

    if( equal_strings( method_name, "link" ) )
        method = LINKED_DISTANCE;
    else if( equal_strings( method_name, "near" ) )
        method = NEAREST_DISTANCE;
    else if( equal_strings( method_name, "normal" ) )
        method = NORMAL_DISTANCE;
    else
    {
        print_error(
          "Usage: %s  input1.obj  input2.obj link|near|normal [output.txt]\n", argv[0] );
        return( 1 );
    }

    outputting = get_string_argument( NULL, &output_filename ) &&
                 !equal_strings( output_filename, "-" );
    (void) get_real_argument( BINTREE_FACTOR, &bintree_factor );

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

    if( method == LINKED_DISTANCE )
    {
        if( !polygons_are_same_topology( polygons1, polygons2 ) )
        {
            print_error(
                 "Polygons must be same topology to used linked method.\n" );
            return( 1 );
        }
    }
    else
    {
        create_polygons_bintree( polygons2,
                                 ROUND( (Real) polygons2->n_items *
                                        bintree_factor ) );
    }

    if( outputting &&
        open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    sum_x = 0.0;
    sum_xx = 0.0;
    min_dist = 0.0;
    max_dist = 0.0;
    n_intersections_not_found = 0;

    compute_polygon_normals( polygons1 );

    for_less( i, 0, polygons1->n_points )
    {
        switch( method )
        {
        case LINKED_DISTANCE:
            dist = distance_between_points( &polygons1->points[i],
                                            &polygons2->points[i] );
            break;

        case NEAREST_DISTANCE:
            (void) find_closest_polygon_point( &polygons1->points[i], polygons2,
                                           &point );
            dist = distance_between_points( &polygons1->points[i], &point );
            break;

        case NORMAL_DISTANCE:
            SCALE_VECTOR( normal, polygons1->normals[i], -1.0 );
            if( intersect_ray_with_object( &polygons1->points[i],
                                           &normal, object_list[0],
                                           &obj_index, &dist, NULL ) == 0 )
            {
                ++n_intersections_not_found;
                dist = 0.0;
            }
            break;
        }

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

    if( n_intersections_not_found > 0 )
        print( "N intersections failed: %d\n", n_intersections_not_found );

    if( outputting )
        (void) close_file( file );

    return( 0 );
}
