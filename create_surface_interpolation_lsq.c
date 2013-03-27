#include  <volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_SMOOTHNESS   1.0

typedef  float   lsq_type;

static  VIO_Real   create_surface_interpolation(
    object_struct    *object,
    int              n_points,
    VIO_Point            points[],
    VIO_Real             values[],
    VIO_Real             smoothness,
    int              n_iters,
    VIO_BOOL          node_values_initialized,
    VIO_Real             node_values[] );

static  void  usage(
    VIO_STR   executable )
{
    static  VIO_STR  usage_str = "\n\
Usage: %s  surface.obj  xyz+values.txt output.txt  [smoothness]\n\
\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               surface_filename, xyz_filename;
    VIO_STR               output_filename, initial_values;
    VIO_File_formats         format;
    FILE                 *file;
    int                  n_objects, n_points, point, n_iters;
    VIO_Point                *points;
    VIO_Real                 *values, value, x, y, z, *node_values, smoothness;
    object_struct        **objects;
    polygons_struct      *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &xyz_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( DEFAULT_SMOOTHNESS, &smoothness );
    (void) get_int_argument( 10, &n_iters );
    (void) get_string_argument( NULL, &initial_values );

    if( input_graphics_file( surface_filename,
                             &format, &n_objects, &objects ) != VIO_OK ||
        n_objects != 1 || get_object_type(objects[0]) != POLYGONS )
    {
        print_error( "File %s must contain 1 polygons object.\n",
                     surface_filename );
        return( 1 );
    }

    n_points = 0;
    points = NULL;
    values = NULL;

    if( open_file( xyz_filename, READ_FILE, ASCII_FORMAT, &file ) != VIO_OK )
        return( 1 );

    while( input_real( file, &x ) == VIO_OK )
    {
        if( input_real( file, &y ) != VIO_OK ||
            input_real( file, &z ) != VIO_OK ||
            input_real( file, &value ) != VIO_OK )
        {
            print_error( "Error reading %s\n", xyz_filename );
            return( 1 );
        }

        SET_ARRAY_SIZE( points, n_points, n_points+1, DEFAULT_CHUNK_SIZE );
        SET_ARRAY_SIZE( values, n_points, n_points+1, DEFAULT_CHUNK_SIZE );

        fill_Point( points[n_points], x, y, z );
        values[n_points] = value;
        ++n_points;
    }

    (void) close_file( file );

    polygons = get_polygons_ptr( objects[0] );
    ALLOC( node_values, polygons->n_points );

    if( initial_values != NULL )
    {
        if( open_file( initial_values, READ_FILE, ASCII_FORMAT, &file ) != VIO_OK )
            return( 1 );

        for_less( point, 0, polygons->n_points )
        {
            if( input_real( file, &node_values[point] ) != VIO_OK )
            {
                print_error( "End of file in values file.\n" );
                return( 1 );
            }
        }

        (void) close_file( file );
    }

    (void) create_surface_interpolation( objects[0], n_points, points, values,
                                         smoothness, n_iters,
                                         initial_values != NULL, node_values );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != VIO_OK )
        return( 1 );

    for_less( point, 0, polygons->n_points )
    {
        if( output_real( file, node_values[point] ) != VIO_OK ||
            output_newline( file  ) != VIO_OK )
            return( 1 );
    }

    (void) close_file( file );

    return( 0 );
}

static  void  create_coefficients(
    VIO_Real             interp_weight,
    VIO_Real             smooth_weight,
    int              n_interp_points,
    VIO_Point            points[],
    VIO_Real             values[],
    object_struct    *object,
    VIO_Real             total_length,
    int              n_neighbours[],
    int              **neighbours,
    VIO_SCHAR     interior_flags[],
    VIO_Real             *constant,
    lsq_type         *linear_terms[],
    lsq_type         *square_terms[],
    int              *n_cross_terms[],
    int              **cross_parms[],
    lsq_type         **cross_terms[] )
{
    int              i, poly, node, p, size;
    int              n_parameters, *indices;
    polygons_struct  *polygons;
    VIO_Point            polygon_points[MAX_POINTS_PER_POLYGON];
    VIO_Point            neigh_points[MAX_POINTS_PER_POLYGON];
    VIO_Real             *weights, dist, avg_dist, weight;
    VIO_Real             x_flat[MAX_POINTS_PER_POLYGON];
    VIO_Real             y_flat[MAX_POINTS_PER_POLYGON];
    VIO_Real             consistency_weights[MAX_POINTS_PER_POLYGON];
    VIO_Point            point_on_surface;

    polygons = get_polygons_ptr( object );

    n_parameters = polygons->n_points;

    initialize_lsq_terms_float( n_parameters, constant, linear_terms,
                                square_terms, n_cross_terms, cross_parms,
                                cross_terms );

    ALLOC( indices, n_parameters );
    ALLOC( weights, n_parameters );

    for_less( p, 0, n_interp_points )
    {
        dist =  find_closest_point_on_object( &points[p], object,
                                              &poly, &point_on_surface );

        if( dist > 1.0 )
            print_error( "Distance too large: ? <%g>\n", dist );
        
        size = get_polygon_points( polygons, poly, polygon_points );

        get_polygon_interpolation_weights( &point_on_surface, size,
                                           polygon_points, weights );

        for_less( i, 0, size )
        {
            indices[i] = polygons->indices[POINT_INDEX(
                                         polygons->end_indices,poly,i)];
            weights[i] *= interp_weight;
        }

        add_to_lsq_terms_float( n_parameters, constant,
                                *linear_terms, *square_terms,
                                *n_cross_terms, *cross_parms, *cross_terms,
                                size, indices, weights,
                                -values[p] * interp_weight, 5 );
    }

    FREE( polygons->normals );
    ALLOC( polygons->normals, 1 );
    FREE( polygons->end_indices );
    ALLOC( polygons->end_indices, 1 );
    FREE( polygons->indices );
    ALLOC( polygons->end_indices, 1 );

    for_less( node, 0, polygons->n_points )
    {
        for_less( p, 0, n_neighbours[node] )
            neigh_points[p] = polygons->points[neighbours[node][p]];

        avg_dist = 0.0;
        for_less( p, 0, n_neighbours[node] )
            avg_dist += distance_between_points( &polygons->points[node],
                                                 &neigh_points[p] );

        avg_dist /= (VIO_Real) n_neighbours[node];

        weight = smooth_weight / sqrt( avg_dist / total_length );

        indices[0] = node;
        weights[0] = weight;

#define FLATTEN
#ifdef  FLATTEN
        flatten_around_vertex( &polygons->points[node],
                               n_neighbours[node], neigh_points,
                               (VIO_BOOL) interior_flags[node],
                               x_flat, y_flat );

        if( !get_interpolation_weights_2d( 0.0, 0.0, n_neighbours[node],
                                           x_flat, y_flat,
                                           consistency_weights ) )
        {
            print_error( "Error in interpolation weights, using avg..\n" );

            for_less( p, 0, n_neighbours[node] )
                consistency_weights[p] = 1.0 / (VIO_Real) n_neighbours[node];
        }
#else
        for_less( p, 0, n_neighbours[node] )
            consistency_weights[p] = 1.0 / (VIO_Real) n_neighbours[node];
#endif

        for_less( p, 0, n_neighbours[node] )
        {
            indices[1+p] = neighbours[node][p];
            weights[1+p] = -weight * consistency_weights[p];
        }

        add_to_lsq_terms_float( n_parameters, constant,
                                *linear_terms, *square_terms,
                                *n_cross_terms, *cross_parms, *cross_terms,
                                n_neighbours[node]+1, indices, weights, 0.0,
                                5 );
    }

    FREE( weights );
    FREE( indices );

    realloc_lsq_terms_float( n_parameters,
                             *n_cross_terms, *cross_parms, *cross_terms );
}

static  VIO_Real   create_surface_interpolation(
    object_struct    *object,
    int              n_interp_points,
    VIO_Point            points[],
    VIO_Real             values[],
    VIO_Real             smoothness,
    int              n_iters,
    VIO_BOOL          node_values_initialized,
    VIO_Real             node_values[] )
{
    polygons_struct   *polygons;
    VIO_Real              total_length, sum_x, sum_xx, variance;
    VIO_Real              dist, interp_weight, smooth_weight, fit, constant;
    int               point, *n_point_neighbours, **point_neighbours;
    int               neigh, n_edges;
    int               n_points;
    VIO_SCHAR      *interior_flags;
    int               *n_cross_terms, **cross_parms;
    lsq_type          *linear_terms, *square_terms, **cross_terms;

    polygons = get_polygons_ptr( object );

    create_polygons_bintree( polygons,
                             (int) ((VIO_Real) polygons->n_items * 0.3) );

    create_polygon_point_neighbours( polygons, FALSE, &n_point_neighbours,
                                     &point_neighbours, &interior_flags, NULL );

    total_length = 0.0;
    n_edges = 0;

    n_points = polygons->n_points;
    for_less( point, 0, n_points )
    {
        n_edges += n_point_neighbours[point];
        for_less( neigh, 0, n_point_neighbours[point] )
        {
            dist = distance_between_points(
                         &polygons->points[point],
                          &polygons->points[point_neighbours[point][neigh]] );
            total_length += dist;
        }
    }

    sum_x = 0.0;
    sum_xx = 0.0;
    for_less( point, 0, n_interp_points )
    {
        sum_x += values[point];
        sum_xx += values[point] * values[point];
    }

    if( n_interp_points == 1 )
        variance = 1.0;
    else
        variance = (sum_xx - sum_x * sum_x / (VIO_Real) n_interp_points) /
                   (VIO_Real) (n_interp_points - 1);

    if( variance == 0.0 )
        variance = 1.0;

    if( !node_values_initialized )
    {
        for_less( point, 0, n_points )
            node_values[point] = sum_x / (VIO_Real) n_interp_points;
    }

    interp_weight = 1.0 / (VIO_Real) n_interp_points / variance;
    smooth_weight = smoothness / (VIO_Real) n_points / variance;

    interp_weight = sqrt( interp_weight );
    smooth_weight = sqrt( smooth_weight );

    create_coefficients( interp_weight, smooth_weight,
                         n_interp_points, points, values,
                         object, total_length,
                         n_point_neighbours, point_neighbours,
                         interior_flags,
                         &constant, &linear_terms, &square_terms,
                         &n_cross_terms, &cross_parms,
                         &cross_terms );

    delete_polygon_point_neighbours( polygons, n_point_neighbours,
                                     point_neighbours, interior_flags, NULL );

    delete_object( object );

    fit = minimize_lsq_float( n_points, constant, linear_terms, square_terms,
                              n_cross_terms, cross_parms, cross_terms,
                              -1.0, n_iters, node_values );

    delete_lsq_terms_float( n_points, linear_terms,
                            square_terms, n_cross_terms, cross_parms,
                            cross_terms );

    return( fit );
}
