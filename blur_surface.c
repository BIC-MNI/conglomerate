#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  generate_neighbours(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[] );

private  void  gaussian_blur_offset(
    int               n_polygon_points,
    Point             points[],
    int               n_neighbours[],
    int               *neighbours[],
    Point             avg_points[],
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
    int              *n_neighbours, **neighbours;
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

    ALLOC( n_neighbours, polygons->n_points );
    ALLOC( neighbours, polygons->n_points );

    generate_neighbours( polygons, n_neighbours, neighbours );

    ALLOC( smooth_points, polygons->n_points );

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Blurring" );

    for_less( i, 0, polygons->n_points )
    {
        gaussian_blur_offset( polygons->n_points, polygons->points,
                              n_neighbours, neighbours,
                              avg->points, i, fwhm, distance,
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

#define  MAX_NEIGHBOURS  10000

private  void  generate_neighbours(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[] )
{
    int              i, point_index, poly, vertex_index, total_n_neighs;
    int              neighs[MAX_NEIGHBOURS], *all_neighs;
    BOOLEAN          dummy;
    progress_struct  progress;

    total_n_neighs = 0;

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Computing Neighbours" );

    for_less( point_index, 0, polygons->n_points )
    {
        if( !find_polygon_with_vertex( polygons, point_index,
                                       &poly, &vertex_index ) )
            handle_internal_error( "generate_neighbours" );

        n_neighbours[point_index] = get_neighbours_of_point(
                                     polygons, poly, vertex_index, neighs,
                                     MAX_NEIGHBOURS, &dummy );

        SET_ARRAY_SIZE( all_neighs, total_n_neighs,
            total_n_neighs + n_neighbours[point_index], DEFAULT_CHUNK_SIZE );

        for_less( i, 0, n_neighbours[point_index] )
        {
            all_neighs[total_n_neighs + i] = neighs[i];
        }

        total_n_neighs += n_neighbours[point_index];

        update_progress_report( &progress, point_index+1 );
    }

    terminate_progress_report( &progress );

    total_n_neighs = 0;
    for_less( point_index, 0, polygons->n_points )
    {
        neighbours[point_index] = &all_neighs[total_n_neighs];

        total_n_neighs += n_neighbours[point_index];
    }
}

private  Real  evaluate_gaussian(
    Real   x,
    Real   e_const )
{
    return( exp( e_const * x * x ) );
}

#define  MAX_NEIGHBOURS   10000

int  get_points_within_dist(
    int               n_polygon_points,
    Point             polygon_points[],
    int               n_neighbours[],
    int               *neighbours[],
    int               point_index,
    Real              dist,
    int               *points[] )
{
    int           current_index, n_points, p, neigh;
    int           i, j;
    Smallest_int  *done_flags;

    n_points = 0;
    current_index = 0;

    ALLOC( done_flags, n_polygon_points );

    for_less( i, 0, n_polygon_points )
        done_flags[i] = FALSE;

    ADD_ELEMENT_TO_ARRAY( *points, n_points, point_index, DEFAULT_CHUNK_SIZE );
    done_flags[point_index] = TRUE;

    while( current_index < n_points )
    {
        p = (*points)[current_index];
        ++current_index;

        for_less( i, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][i];

            if( done_flags[neigh] )
                continue;

            done_flags[neigh] = TRUE;

            if( distance_between_points( &polygon_points[point_index],
                                         &polygon_points[neigh] ) <=
                dist )
            {
                ADD_ELEMENT_TO_ARRAY( *points, n_points, neigh,
                                      DEFAULT_CHUNK_SIZE );
            }
        }
    }

    FREE( done_flags );

    return( n_points );
}

private  void  gaussian_blur_offset(
    int               n_polygon_points,
    Point             polygon_points[],
    int               n_neighbours[],
    int               *neighbours[],
    Point             avg_points[],
    int               point_index,
    Real              fwhm,
    Real              dist,
    Point             *smooth_point )
{
    Real   sum[3], weight, sum_weight, offset, point_dist, e_const;
    int    i, c, n_points, *points, neigh;

    n_points = get_points_within_dist( n_polygon_points, polygon_points,
                                       n_neighbours, neighbours,
                                       point_index, dist * fwhm, &points );

    sum[0] = 0.0;
    sum[1] = 0.0;
    sum[2] = 0.0;
    sum_weight = 0.0;

    e_const = log( 0.5 ) / fwhm / fwhm;

    for_less( i, 0, n_points )
    {
        neigh = points[i];

        point_dist = distance_between_points( &polygon_points[point_index],
                                              &polygon_points[neigh] );

        weight = evaluate_gaussian( point_dist, e_const );

        for_less( c, 0, N_DIMENSIONS )
        {
            offset = Point_coord(polygon_points[neigh],c) -
                     Point_coord(avg_points[neigh],c);

            sum[c] += offset * weight;
        }

        sum_weight += weight;
    }

    for_less( c, 0, N_DIMENSIONS )
    {
        Point_coord(*smooth_point,c) = Point_coord(avg_points[point_index],c) +                                       sum[c] / sum_weight;
    }
}
