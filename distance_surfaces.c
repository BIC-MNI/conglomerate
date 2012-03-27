#include  <volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.3

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input1_filename, input2_filename;
    int              i, n_objects, which, n_samples, obj_index, n_intersections;
    File_formats     format;
    object_struct    **object_list[2];
    polygons_struct  *polygons[2];
    Point            point;
    Vector           direction;
    Real             avg, var, sum_x, sum_xx, dist, std_dev;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) )
    {
        print_error(
          "Usage: %s  input1.obj  input2.obj\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input1_filename, &format, &n_objects,
                             &object_list[0] ) != OK || n_objects != 1 ||
        get_object_type(object_list[0][0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input1_filename );
        return( 1 );
    }

    polygons[0] = get_polygons_ptr( object_list[0][0] );

    if( input_graphics_file( input2_filename, &format, &n_objects,
                             &object_list[1] ) != OK || n_objects != 1 ||
        get_object_type(object_list[1][0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input2_filename );
        return( 1 );
    }

    polygons[1] = get_polygons_ptr( object_list[1][0] );


    n_samples = 0;
    sum_x = 0.0;
    sum_xx = 0.0;

    for_less( which, 0, 2 )
    {
        create_polygons_bintree( polygons[1-which],
                                 ROUND( (Real) polygons[1-which]->n_items *
                                        BINTREE_FACTOR ) );

        for_less( i, 0, polygons[which]->n_points )
        {
            (void) find_closest_polygon_point( &polygons[which]->points[i],
                                               polygons[1-which],
                                               &point );

            SUB_POINTS( direction, point, polygons[which]->points[i] );
            NORMALIZE_VECTOR( direction, direction );
            n_intersections = intersect_ray_with_object(
                           &polygons[which]->points[i], &direction,
                           object_list[1-which][0], &obj_index, &dist, NULL );

            dist = distance_between_points( &polygons[which]->points[i],
                                            &point );

            if( (n_intersections % 2) != which )
                dist = -dist;


            sum_x += dist;
            sum_xx += dist * dist;
            ++n_samples;
        }

        delete_bintree_if_any( &polygons[1-which]->bintree );
    }

    avg = sum_x / (Real) n_samples;
    var = (sum_xx - sum_x * sum_x / (Real) n_samples) / (Real) (n_samples-1);

    if( var > 0.0 )
        std_dev = sqrt( var );
    else
        std_dev = 0.0;

    print( "Average Distance: %g\n", avg );
    print( "        Std. Dev: %g\n", std_dev );

    return( 0 );
}
