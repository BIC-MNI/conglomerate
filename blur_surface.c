#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

private  void  gaussian_blur_offset(
    polygons_struct   *polygons,
    polygons_struct   *avg,
    int               point_index,
    Real              fwhm,
    Real              dist,
    Point             *smooth_point );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, avg_filename, output_filename;
    int              i, n_objects;
    File_formats     format;
    object_struct    **object_list, **avg_object_list;
    polygons_struct  *polygons, *avg;
    Point            *smooth_points;
    Real             fwhm, distance;
    progress_struct  progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &avg_filename ) ||
        !get_real_argument( 0.0, &fwhm ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
          "Usage: %s  input.obj  avg.obj fwhm out.obj\n", argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 3.0, &distance );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input_filename );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( avg_filename, &format, &n_objects,
                             &avg_object_list ) != OK || n_objects != 1 ||
        get_object_type(avg_object_list[0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", avg_filename );
        return( 1 );
    }

    avg = get_polygons_ptr( avg_object_list[0] );

    if( !polygons_are_same_topology( polygons, avg ) )
    {
        print_error( "Polygons are not the same topology.\n" );
        return( 1 );
    }

    check_polygons_neighbours_computed( polygons );

    ALLOC( smooth_points, polygons->n_points );

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Blurring" );

    for_less( i, 0, polygons->n_points )
    {
        gaussian_blur_offset( polygons, avg, i, fwhm, distance,
                              &smooth_points[i] );

        update_progress_report( &progress, i+1 );
    }

    terminate_progress_report( &progress );

    FREE( polygons->points );
    polygons->points = smooth_points;

    compute_polygon_normals( polygons );

    output_graphics_file( output_filename, format, 1, object_list );

    return( 0 );
}

private  Real  evaluate_gaussian(
    Real   x,
    Real   e_const )
{
    return( exp( e_const * x * x ) );
}

#define  MAX_NEIGHBOURS   10000

int  get_points_within_dist(
    polygons_struct   *polygons,
    int               point_index,
    Real              dist,
    int               *points[] )
{
    int      current_index, n_points, poly, vertex_index, p, n_neighs;
    int      i, j, neighbours[MAX_NEIGHBOURS];
    BOOLEAN  dummy;

    n_points = 0;
    current_index = 0;

    ADD_ELEMENT_TO_ARRAY( *points, n_points, point_index, DEFAULT_CHUNK_SIZE );

    while( current_index < n_points )
    {
        p = (*points)[current_index];
        ++current_index;

        if( !find_polygon_with_vertex( polygons, p, &poly, &vertex_index ) )
            handle_internal_error( "get_points_within_dist" );

        n_neighs = get_neighbours_of_point( polygons, poly, vertex_index,
                                            neighbours, MAX_NEIGHBOURS,
                                            &dummy );

        for_less( i, 0, n_neighs )
        {
            if( distance_between_points( &polygons->points[point_index],
                                         &polygons->points[neighbours[i]] ) <=
                dist )
            {
                for_less( j, 0, n_points )
                {
                    if( (*points)[j] == neighbours[i] )
                        break;
                }

                if( j == n_points )
                {
                    ADD_ELEMENT_TO_ARRAY( *points, n_points, neighbours[i],
                                          DEFAULT_CHUNK_SIZE );
                }
            }
        }
    }

    return( n_points );
}

private  void  gaussian_blur_offset(
    polygons_struct   *polygons,
    polygons_struct   *avg,
    int               point_index,
    Real              fwhm,
    Real              dist,
    Point             *smooth_point )
{
    Real   sum[3], weight, sum_weight, offset, point_dist, e_const;
    int    i, c, n_points, *points, neigh;

    n_points = get_points_within_dist( polygons, point_index, dist * fwhm,
                                       &points );

    sum[0] = 0.0;
    sum[1] = 0.0;
    sum[2] = 0.0;
    sum_weight = 0.0;

    e_const = log( 0.5 ) / fwhm / fwhm;

    for_less( i, 0, n_points )
    {
        neigh = points[i];

        point_dist = distance_between_points( &polygons->points[point_index],
                                              &polygons->points[neigh] );

        weight = evaluate_gaussian( point_dist, e_const );

        for_less( c, 0, N_DIMENSIONS )
        {
            offset = Point_coord(polygons->points[neigh],c) -
                     Point_coord(avg->points[neigh],c);

            sum[c] += offset * weight;
        }

        sum_weight += weight;
    }

    for_less( c, 0, N_DIMENSIONS )
    {
        Point_coord(*smooth_point,c) = Point_coord(avg->points[point_index],c) +                                       sum[c] / sum_weight;
    }
}
