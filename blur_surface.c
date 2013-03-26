#include  <volume_io.h>
#include  <bicpl.h>
#include  <special_geometry.h>

static  void  gaussian_blur_points(
    int               n_polygon_points,
    VIO_Point             points[],
    VIO_Real              values[],
    int               n_neighbours[],
    int               *neighbours[],
    VIO_SCHAR      done_flags[],
    int               point_index,
    VIO_Real              fwhm,
    VIO_Real              dist,
    VIO_Point             *smooth_point,
    VIO_Real              *value );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR           input_filename, output_filename, values_filename;
    int              n_objects, p;
    int              *n_neighbours, **neighbours;
    FILE             *file;
    VIO_File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons;
    VIO_SCHAR     *done_flags;
    VIO_Point            *smooth_points, point;
    VIO_Real             fwhm, distance_ratio, *values, value, *smooth_values;
    VIO_BOOL          values_present;
    VIO_progress_struct  progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &fwhm ) ||
        !get_real_argument( 3.0, &distance_ratio ) )
    {
        print_error(
          "Usage: %s  input.obj  output.obj fwhm  dist_ratio [values]\n",
                      argv[0] );
        return( 1 );
    }

    values_present = get_string_argument( NULL, &values_filename );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input_filename );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    if( values_present )
    {
        ALLOC( values, polygons->n_points );
        ALLOC( smooth_values, polygons->n_points );

        if( open_file( values_filename, READ_FILE, ASCII_FORMAT, &file ) != VIO_OK )
            return( 1 );

        for_less( p, 0, polygons->n_points )
        {
            if( input_real( file, &values[p] ) != VIO_OK )
                return( 1 );
        }

        (void) close_file( file );

        smooth_points = NULL;
    }
    else
    {
        ALLOC( smooth_points, polygons->n_points );
        values = NULL;
    }

    check_polygons_neighbours_computed( polygons );

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    ALLOC( done_flags, polygons->n_points );

    for_less( p, 0, polygons->n_points )
        done_flags[p] = FALSE;

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Blurring" );

    for_less( p, 0, polygons->n_points )
    {
        gaussian_blur_points( polygons->n_points, polygons->points, values,
                              n_neighbours, neighbours, done_flags,
                              p, fwhm, distance_ratio, &point, &value );

        if( values_present )
            smooth_values[p] = value;
        else
            smooth_points[p] = point;

        update_progress_report( &progress, p+1 );
    }

    terminate_progress_report( &progress );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     NULL, NULL );

    if( values_present )
    {
        if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != VIO_OK)
            return( 1 );

        for_less( p, 0, polygons->n_points )
        {
            if( output_real( file, smooth_values[p] ) != VIO_OK ||
                output_newline( file ) != VIO_OK )
                return( 1 );
        }

        (void) close_file( file );
    }
    else
    {
        FREE( polygons->points );
        polygons->points = smooth_points;

        compute_polygon_normals( polygons );

        output_graphics_file( output_filename, format, 1, object_list );
    }

    return( 0 );
}

static  VIO_Real  evaluate_gaussian(
    VIO_Real   x,
    VIO_Real   e_const )
{
    return( exp( e_const * x * x ) );
}

#define  MAX_NEIGHBOURS   10000

int  get_points_within_dist(
    int               n_polygon_points,
    VIO_Point             polygon_points[],
    int               n_neighbours[],
    int               *neighbours[],
    VIO_SCHAR      done_flags[],
    int               point_index,
    VIO_Real              dist,
    int               points[],
    VIO_Real              dists[] )
{
    int           current_index, n_points, p, neigh, i;
    VIO_Real          this_dist;

    current_index = 0;

    points[0] = point_index;
    dists[0] = 0.0;
    done_flags[point_index] = TRUE;
    n_points = 1;

    while( current_index < n_points )
    {
        p = points[current_index];
        ++current_index;

        for_less( i, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][i];

            if( done_flags[neigh] )
                continue;

            done_flags[neigh] = TRUE;

            this_dist = distance_between_points(
                                 &polygon_points[point_index],
                                 &polygon_points[neigh] );
            if( this_dist <= dist )
            {
                points[n_points] = neigh;
                dists[n_points] = this_dist;
                ++n_points;
            }
        }
    }

    for_less( i, 0, n_points )
        done_flags[points[i]] = FALSE;

    return( n_points );
}

static  void  gaussian_blur_points(
    int               n_polygon_points,
    VIO_Point             polygon_points[],
    VIO_Real              values[],
    int               n_neighbours[],
    int               *neighbours[],
    VIO_SCHAR      *done_flags,
    int               point_index,
    VIO_Real              fwhm,
    VIO_Real              dist,
    VIO_Point             *smooth_point,
    VIO_Real              *value )
{
    VIO_Real   sum[3], weight, sum_weight, point_dist, e_const;
    VIO_Real   *dists;
    int    i, c, n_points, *points, neigh;

    ALLOC( points, n_polygon_points );
    ALLOC( dists, n_polygon_points );

    n_points = get_points_within_dist( n_polygon_points, polygon_points,
                                       n_neighbours, neighbours, done_flags,
                                       point_index, dist * fwhm, points,
                                       dists );

    sum[0] = 0.0;
    sum[1] = 0.0;
    sum[2] = 0.0;
    sum_weight = 0.0;

    if( fwhm <= 0.0 )
        fwhm = 1e-20;

    e_const = log( 0.5 ) / (fwhm/2.0 * fwhm/2.0);

    for_less( i, 0, n_points )
    {
        neigh = points[i];

        point_dist = dists[i];

        weight = evaluate_gaussian( point_dist, e_const );

        if( values != NULL )
            sum[0] += weight * values[neigh];
        else
        {
            for_less( c, 0, VIO_N_DIMENSIONS )
                sum[c] += weight * (VIO_Real) Point_coord(polygon_points[neigh],c);
        }

        sum_weight += weight;
    }

    FREE( points );
    FREE( dists );

    if( values != NULL )
    {
        *value = sum[0] / sum_weight;
    }
    else
    {
        for_less( c, 0, VIO_N_DIMENSIONS )
            Point_coord(*smooth_point,c) = (VIO_Point_coord_type) (sum[c] /
                                                               sum_weight);
    }
}
