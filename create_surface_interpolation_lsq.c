#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_SMOOTHNESS   1.0

private  Real   create_surface_interpolation(
    object_struct    *object,
    int              n_points,
    Point            points[],
    Real             values[],
    Real             smoothness,
    int              n_iters,
    BOOLEAN          node_values_initialized,
    Real             node_values[] );

private  void  usage(
    STRING   executable )
{
    static  STRING  usage_str = "\n\
Usage: %s  surface.obj  xyz+values.txt output.txt  [smoothness]\n\
\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               surface_filename, xyz_filename;
    STRING               output_filename, initial_values;
    File_formats         format;
    FILE                 *file;
    int                  n_objects, n_points, point, n_iters;
    Point                *points;
    Real                 *values, value, x, y, z, *node_values, smoothness;
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
                             &format, &n_objects, &objects ) != OK ||
        n_objects != 1 || get_object_type(objects[0]) != POLYGONS )
    {
        print_error( "File %s must contain 1 polygons object.\n",
                     surface_filename );
        return( 1 );
    }

    n_points = 0;
    points = NULL;
    values = NULL;

    if( open_file( xyz_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    while( input_real( file, &x ) == OK )
    {
        if( input_real( file, &y ) != OK ||
            input_real( file, &z ) != OK ||
            input_real( file, &value ) != OK )
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
        if( open_file( initial_values, READ_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );

        for_less( point, 0, polygons->n_points )
        {
            if( input_real( file, &node_values[point] ) != OK )
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

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( point, 0, polygons->n_points )
    {
        if( output_real( file, node_values[point] ) != OK ||
            output_newline( file  ) != OK )
            return( 1 );
    }

    (void) close_file( file );

    return( 0 );
}

private  int  create_coefficients(
    Real             interp_weight,
    Real             smooth_weight,
    int              n_interp_points,
    Point            points[],
    Real             values[],
    object_struct    *object,
    Real             total_length,
    int              n_neighbours[],
    int              **neighbours,
    Smallest_int     interior_flags[],
    int              *n_nodes_involved[],
    int              **node_list[],
    Real             *constants[],
    Real             **node_weights[] )
{
    int              i, n_equations, eq, poly, node, p, size;
    polygons_struct  *polygons;
    Point            polygon_points[MAX_POINTS_PER_POLYGON];
    Point            neigh_points[MAX_POINTS_PER_POLYGON];
    Real             weights[MAX_POINTS_PER_POLYGON], dist, avg_dist, weight;
    Real             x_flat[MAX_POINTS_PER_POLYGON];
    Real             y_flat[MAX_POINTS_PER_POLYGON];
    Real             consistency_weights[MAX_POINTS_PER_POLYGON];
    Point            point_on_surface;

    polygons = get_polygons_ptr( object );

    n_equations = polygons->n_points + n_interp_points;

    ALLOC( *n_nodes_involved, n_equations );
    ALLOC( *constants, n_equations );
    ALLOC( *node_weights, n_equations );
    ALLOC( *node_list, n_equations );

    eq = 0;

    for_less( p, 0, n_interp_points )
    {
        dist =  find_closest_point_on_object( &points[p], object,
                                              &poly, &point_on_surface );

        if( dist > 1.0 )
            print_error( "Distance too large: ? <%g>\n", dist );
        
        size = get_polygon_points( polygons, poly, polygon_points );

        (*n_nodes_involved)[eq] = size;
        ALLOC( (*node_list)[eq], size );
        ALLOC( (*node_weights)[eq], size );

        (*constants)[eq] = - values[p] * interp_weight;

        get_polygon_interpolation_weights( &point_on_surface, size,
                                           polygon_points, weights );

        for_less( i, 0, size )
        {
            (*node_list)[eq][i] = polygons->indices[POINT_INDEX(
                                         polygons->end_indices,poly,i)];
            (*node_weights)[eq][i] = weights[i] * interp_weight;
        }

        ++eq;
    }

    for_less( node, 0, polygons->n_points )
    {
        for_less( p, 0, n_neighbours[node] )
            neigh_points[p] = polygons->points[neighbours[node][p]];

        avg_dist = 0.0;
        for_less( p, 0, n_neighbours[node] )
            avg_dist += distance_between_points( &polygons->points[node],
                                                 &neigh_points[p] );

        avg_dist /= (Real) n_neighbours[node];

        weight = smooth_weight / sqrt( avg_dist / total_length );

        (*constants)[eq] = 0.0;
        (*n_nodes_involved)[eq] = 1 + n_neighbours[node];
        ALLOC( (*node_list)[eq], (*n_nodes_involved)[eq] );
        ALLOC( (*node_weights)[eq], (*n_nodes_involved)[eq] );

        (*node_list)[eq][0] = node;
        (*node_weights)[eq][0] = weight;

#define FLATTEN
#ifdef  FLATTEN
        flatten_around_vertex( &polygons->points[node],
                               n_neighbours[node], neigh_points,
                               (BOOLEAN) interior_flags[node],
                               x_flat, y_flat );

        if( !get_interpolation_weights_2d( 0.0, 0.0, n_neighbours[node],
                                           x_flat, y_flat,
                                           consistency_weights ) )
        {
            print_error( "Error in interpolation weights, using avg..\n" );

            for_less( p, 0, n_neighbours[node] )
                consistency_weights[p] = 1.0 / (Real) n_neighbours[node];
        }
#else
        for_less( p, 0, n_neighbours[node] )
            consistency_weights[p] = 1.0 / (Real) n_neighbours[node];
#endif

        for_less( p, 0, n_neighbours[node] )
        {
            (*node_list)[eq][1+p] = neighbours[node][p];
            (*node_weights)[eq][1+p] = -weight * consistency_weights[p];
        }

        ++eq;
    }

    return( n_equations );
}

private  Real   create_surface_interpolation(
    object_struct    *object,
    int              n_points,
    Point            points[],
    Real             values[],
    Real             smoothness,
    int              n_iters,
    BOOLEAN          node_values_initialized,
    Real             node_values[] )
{
    polygons_struct   *polygons;
    Real              total_length, sum_x, sum_xx, variance;
    Real              dist, interp_weight, smooth_weight, fit;
    int               point, *n_point_neighbours, **point_neighbours;
    int               neigh, n_edges;
    int               n_equations, *n_nodes_per_equation, **node_list;
    Real              **node_weights, *constants;
    Smallest_int      *interior_flags;

    polygons = get_polygons_ptr( object );

    create_polygons_bintree( polygons,
                             (int) ((Real) polygons->n_items * 0.3) );

    create_polygon_point_neighbours( polygons, FALSE, &n_point_neighbours,
                                     &point_neighbours, &interior_flags, NULL );

    total_length = 0.0;
    n_edges = 0;

    for_less( point, 0, polygons->n_points )
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
    for_less( point, 0, n_points )
    {
        sum_x += values[point];
        sum_xx += values[point] * values[point];
    }

    if( n_points == 1 )
        variance = 1.0;
    else
        variance = (sum_xx - sum_x * sum_x / (Real) n_points) /
                   (Real) (n_points - 1);

    if( variance == 0.0 )
        variance = 1.0;

    if( !node_values_initialized )
    {
        for_less( point, 0, polygons->n_points )
            node_values[point] = sum_x / (Real) n_points;
    }

    interp_weight = 1.0 / (Real) n_points / variance;
    smooth_weight = smoothness / (Real) polygons->n_points / variance;

    interp_weight = sqrt( interp_weight );
    smooth_weight = sqrt( smooth_weight );

    n_equations = create_coefficients( interp_weight, smooth_weight,
                                       n_points, points, values,
                                       object, total_length,
                                       n_point_neighbours, point_neighbours,
                                       interior_flags,
                                       &n_nodes_per_equation,
                                       &node_list, &constants, &node_weights );

    fit = minimize_lsq( polygons->n_points, n_equations, n_nodes_per_equation,
                        node_list, constants, node_weights, n_iters,
                        node_values );

    for_less( point, 0, n_equations )
    {
        FREE( node_weights[point] );
        FREE( node_list[point] );
    }
    FREE( node_weights );
    FREE( node_list );
    FREE( n_nodes_per_equation );
    FREE( constants );

    delete_polygon_point_neighbours( polygons, n_point_neighbours,
                                     point_neighbours, interior_flags, NULL );

    return( fit );
}
