#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_SMOOTHNESS   1.0

private  void   create_surface_interpolation(
    object_struct    *object,
    int              n_points,
    Point            points[],
    Real             values[],
    Real             smoothness,
    int              max_neighbours,
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
    int                  n_objects, n_points, point, n_iters, max_neighbours;
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
    (void) get_int_argument( 1, &max_neighbours );
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
                return( 1 );
        }

        (void) close_file( file );
    }

    create_surface_interpolation( objects[0], n_points, points, values,
                                  smoothness, max_neighbours, n_iters,
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

private  Real  evaluate_fit(
    int    n_neighbours[],
    int    *neighbours[],
    Real   interp_weight,
    Real   smooth_weight,
    int    n_interp_points,
    int    n_weights[],
    Real   **weights,
    int    **weighted_points,
    Real   values[],
    float  **distances,
    Real   total_length,
    int    n_nodes,
    Point  nodes[],
    Real   node_values[],
    Real   *fit1,
    Real   *fit2 )
{
    int   node, p, i;
    Real  val, diff, f1, f2, avg, avg_dist;

    f1 = 0.0;

    for_less( p, 0, n_interp_points )
    {
        val = 0.0;
        for_less( i, 0, n_weights[p] )
            val += weights[p][i] * node_values[weighted_points[p][i]];

        diff = (val - values[p]) * interp_weight;
        f1 += diff * diff;
    }

    f2 = 0.0;

    for_less( node, 0, n_nodes )
    {
        avg = 0.0;
        avg_dist = 0.0;
        for_less( p, 0, n_neighbours[node] )
        {
            avg += node_values[neighbours[node][p]];
            avg_dist += (Real) distances[node][p];
        }

        avg_dist /= (Real) n_neighbours[node] * total_length;
        diff = smooth_weight *
               (node_values[node] - avg / (Real) n_neighbours[node]) /
               avg_dist;
        f2 += diff * diff;
    }

    if( fit1 != NULL ) *fit1 = f1;
    if( fit2 != NULL ) *fit2 = f2;

    return( f1 + f2 );
}

private  void  fast_minimize_nodes(
    int    node,
    int    *n_neighbours,
    int    *neighbours[],
    Real   interp_weight,
    Real   smooth_weight,
    int    n_interp_points,
    int    n_weights[],
    Real   **weights,
    int    **weighted_points,
    Real   values[],
    float  **distances,
    Real   total_length,
    int    n_nodes,
    Point  nodes[],
    Real   node_values[] )
{
    int                    p, i, which, neigh_index, current_node;
    Real                   linear, constant, a, b, avg_dist;

    a = 0.0;
    b = 0.0;

    for_less( p, 0, n_interp_points )
    {
        for_less( which, 0, n_weights[p] )
        {
            if( weighted_points[p][which] == node )
                break;
        }

        if( which >= n_weights[p] )
            continue;

        constant = -values[p];
        linear = 0.0;
        for_less( i, 0, n_weights[p] )
        {
            if( weighted_points[p][i] == node )
                linear += weights[p][i];
            else
                constant += weights[p][i] * node_values[weighted_points[p][i]];
        }

        constant *= interp_weight;
        linear *= interp_weight;

        a += linear * linear;
        b += 2.0 * linear * constant;
    }

    for_less( which, 0, 1 + n_neighbours[node] )
    {
        if( which == n_neighbours[node] )
        {
            current_node = node;
            linear = 1.0;
            constant = 0.0;
        }
        else
        {
            current_node = neighbours[node][which];
            linear = 0.0;
            constant = node_values[current_node];
        }

        avg_dist = 0.0;
        for_less( neigh_index, 0, n_neighbours[current_node] )
            avg_dist += (Real) distances[current_node][neigh_index];
        avg_dist /= (Real) n_neighbours[node] * total_length;

        for_less( neigh_index, 0, n_neighbours[current_node] )
        {
            if( neighbours[current_node][neigh_index] == node )
                linear -= 1.0 / (Real) n_neighbours[current_node];
            else
                constant -= node_values[neighbours[current_node][neigh_index]] /
                                (Real) n_neighbours[current_node];
        }

        linear *= smooth_weight / avg_dist;
        constant *= smooth_weight / avg_dist;
        a += linear * linear;
        b += 2.0 * linear * constant;
    }

    if( a == 0.0 )
        print_error( "Error getting least squares.\n" );

    node_values[node] = -b / (2.0*a);
}

#ifdef OLD
private  void  minimize_nodes(
    int    n_node_indices,
    int    node_indices[],
    int    *n_neighbours,
    int    *neighbours[],
    Real   interp_weight,
    Real   smooth_weight,
    int    n_interp_points,
    int    n_weights[],
    Real   **weights,
    int    **weighted_points,
    Real   values[],
    float  **distances,
    Real   total_length,
    int    n_nodes,
    Point  nodes[],
    Real   node_values[] )
{
    int                    p, i, n, neigh_index, neigh_position, node;
    Real                   constant, dist;
    Real                   *coefs;
    linear_least_squares   lsq;
    BOOLEAN                neigh_also_in_list;

    ALLOC( coefs, n_node_indices );

    initialize_linear_least_squares( &lsq, n_node_indices );

    for_less( p, 0, n_interp_points )
    {
        for_less( i, 0, n_weights[p] )
        {
            for_less( n, 0, n_node_indices )
            {
                if( weighted_points[p][i] == node_indices[n] )
                    break;
            }

            if( n < n_node_indices )
                break;
        }

        if( i >= n_weights[p] )
            continue;

        for_less( n, 0, n_node_indices )
            coefs[n] = 0.0;
        constant = interp_weight * values[p];

        for_less( i, 0, n_weights[p] )
        {
            for_less( n, 0, n_node_indices )
            {
                if( weighted_points[p][i] == node_indices[n] )
                    break;
            }

            if( n < n_node_indices )
                coefs[n] = weights[p][i] * interp_weight;
            else
                constant -= weights[p][i] * interp_weight *
                            node_values[weighted_points[p][i]];
        }

        add_to_linear_least_squares( &lsq, coefs, constant );
    }

    for_less( n, 0, n_node_indices )
    {
        node = node_indices[n];
        for_less( neigh_index, 0, n_neighbours[node] )
        {
            for_less( p, 0, n_node_indices )
            {
                if( node == neighbours[node][neigh_index] )
                    break;
            }
            neigh_position = p;
            neigh_also_in_list = (p < n_node_indices);

            for_less( p, 0, n_node_indices )
                coefs[p] = 0.0;

dist = 0.0;

            coefs[n] = smooth_weight;
            if( neigh_also_in_list )
            {
                coefs[neigh_position] = -smooth_weight;
                constant = 0.0;
            }
            else
                constant = smooth_weight *
                           node_values[neighbours[node][neigh_index]] / dist;

            add_to_linear_least_squares( &lsq, coefs, constant );
        }
    }

    if( !get_linear_least_squares_solution( &lsq, coefs ) )
        print_error( "Error getting least squares.\n" );

    for_less( n, 0, n_node_indices )
        node_values[node_indices[n]] = coefs[n];

    delete_linear_least_squares( &lsq );

    FREE( coefs );
}
#endif

#define  INCREMENT  1.0e-4

private  void  minimize_cost(
    Real   interp_weight,
    Real   smooth_weight,
    int    n_interp_points,
    int    n_weights[],
    Real   **weights,
    int    **weighted_points,
    Real   values[],
    float  **distances,
    Real   total_length,
    int    n_nodes,
    Point  nodes[],
    Real   node_values[],
    int    n_neighbours[],
    int    *neighbours[],
    int    max_neighbours )
{
    int     node;
/*
    Real    fit0, fit1, fit2, fit3, save, fa, fb;
*/

    for_less( node, 0, n_nodes )
    {
/*
        fit0 = evaluate_fit( n_neighbours, neighbours,
                             interp_weight, smooth_weight, n_interp_points,
                             n_weights, weights, weighted_points, values,
                             distances, total_length, n_nodes, nodes,
                             node_values, NULL, NULL );
*/

#ifdef OLD
        n_to_do = 1;
        list[0] = node;

        if( max_neighbours > 1 )
        {
            n_to_do += n_neighbours[node];
            for_less( n, 0, n_neighbours[node] )
                list[n+1] = neighbours[node][n];
        }

        if( max_neighbours >= n_nodes )
        {
            n_to_do = n_nodes;
            for_less( n, 0, n_nodes )
                list[n] = n;
        }

        if( n_to_do == 1 )
#endif
        {
            fast_minimize_nodes( node, n_neighbours, neighbours,
                interp_weight, smooth_weight,
                n_interp_points, n_weights, weights, weighted_points,
                values, distances, total_length, n_nodes, nodes, node_values );
        }
#ifdef OLD
        else
        {
            minimize_nodes( n_to_do, list, n_neighbours, neighbours,
                interp_weight, smooth_weight,
                n_interp_points, n_weights, weights, weighted_points,
                values, distances, total_length, n_nodes, nodes, node_values );
        }

        if( n_to_do == n_nodes )
            break;
#endif

/*
        save = node_values[node];

        fit2 = evaluate_fit( n_neighbours, neighbours,
                             interp_weight, smooth_weight, n_interp_points,
                             n_weights, weights, weighted_points, values,
                             distances, total_length, n_nodes, nodes,
                             node_values, &fa, &fb );

        node_values[node] = save - INCREMENT;
        fit1 = evaluate_fit( n_neighbours, neighbours,
                             interp_weight, smooth_weight, n_interp_points,
                             n_weights, weights, weighted_points, values,
                             distances, total_length, n_nodes, nodes,
                             node_values, NULL, NULL );

        node_values[node] = save + INCREMENT;
        fit3 = evaluate_fit( n_neighbours, neighbours,
                             interp_weight, smooth_weight, n_interp_points,
                             n_weights, weights, weighted_points, values,
                             distances, total_length, n_nodes, nodes,
                             node_values, NULL, NULL );

        if( fit1 < fit2 || fit3 < fit2 )
        {
            print_error( "Error: (%g)   %g ## %g    (%g)\n", fit1, fit0, fit2, fit3 );
        }

        node_values[node] = save;
*/
    }
}

private  void   create_surface_interpolation(
    object_struct    *object,
    int              n_points,
    Point            points[],
    Real             values[],
    Real             smoothness,
    int              max_neighbours,
    int              n_iters,
    BOOLEAN          node_values_initialized,
    Real             node_values[] )
{
    polygons_struct   *polygons;
    Real              total_length, sum_x, sum_xx, std_dev;
    Point             polygon_points[MAX_POINTS_PER_POLYGON];
    Point             point_on_surface;
    Real              **weights;
    Real              dist, interp_weight, smooth_weight, fit1, fit2;
    float             **distances;
    int               point, *n_point_neighbours, **point_neighbours, p;
    int               iter, size, obj_index, neigh, n_edges;
    int               **weighted_points, *n_weights;
    int               next_change, update_rate;

    polygons = get_polygons_ptr( object );

    create_polygons_bintree( polygons,
                             (int) ((Real) polygons->n_items * 0.3) );

    create_polygon_point_neighbours( polygons, &n_point_neighbours,
                                     &point_neighbours, NULL );

    ALLOC( distances, polygons->n_points );

    total_length = 0.0;
    n_edges = 0;

    for_less( point, 0, polygons->n_points )
    {
        ALLOC( distances[point], n_point_neighbours[point] );
        n_edges += n_point_neighbours[point];
        for_less( neigh, 0, n_point_neighbours[point] )
        {
            distances[point][neigh] = (float) distance_between_points(
                         &polygons->points[point],
                          &polygons->points[point_neighbours[point][neigh]] );
            total_length += (Real) distances[point][neigh];
        }
    }

    total_length /= (Real) n_edges;

    ALLOC( n_weights, n_points );
    ALLOC( weights, n_points );
    ALLOC( weighted_points, n_points );

    sum_x = 0.0;
    sum_xx = 0.0;
    for_less( point, 0, n_points )
    {
        sum_x += values[point];
        sum_xx += values[point] * values[point];
        dist =  find_closest_point_on_object( &points[point], object,
                                              &obj_index, &point_on_surface );

        if( dist > 1.0 )
            print_error( "Distance too large: ? <%g>\n", dist );

        
        size = get_polygon_points( polygons, obj_index, polygon_points );

        n_weights[point] = size;
        ALLOC( weights[point], size );
        ALLOC( weighted_points[point], size );

        for_less( p, 0, size )
        {
            weighted_points[point][p] = polygons->indices[
                   POINT_INDEX(polygons->end_indices,obj_index,p)];
        }

        get_polygon_interpolation_weights( &point_on_surface, size,
                                           polygon_points, weights[point] );
    }

    if( n_points == 1 )
        std_dev = 1.0;
    else
        std_dev = (sum_xx - sum_x * sum_x / (Real) n_points) /
                  (Real) (n_points - 1);

    if( std_dev == 0.0 )
        std_dev = 1.0;
    else
        std_dev = sqrt( std_dev );

    if( !node_values_initialized )
    {
        for_less( point, 0, polygons->n_points )
            node_values[point] = sum_x / (Real) n_points;
    }

    interp_weight = 1.0 / (Real) n_points / std_dev;
    smooth_weight = smoothness / (Real) polygons->n_points;

    interp_weight = sqrt( interp_weight );
    smooth_weight = sqrt( smooth_weight );

    (void) evaluate_fit( n_point_neighbours, point_neighbours,
                         interp_weight, smooth_weight, n_points,
                         n_weights, weights, weighted_points, values,
                         distances, total_length, polygons->n_points,
                         polygons->points,
                         node_values, &fit1, &fit2 );

    print( "Initial  %g %g\n", fit1, fit2 );

    update_rate = 1;
    next_change = 10;
    for_less( iter, 0, n_iters )
    {
        minimize_cost( interp_weight, smooth_weight,
                       n_points, n_weights, weights, weighted_points, values,
                       distances, total_length,
                       polygons->n_points, polygons->points, node_values,
                       n_point_neighbours, point_neighbours, max_neighbours );

        (void) evaluate_fit( n_point_neighbours, point_neighbours,
                      interp_weight, smooth_weight, n_points,
                      n_weights, weights, weighted_points, values,
                      distances, total_length,
                      polygons->n_points, polygons->points,
                      node_values, &fit1, &fit2 );

        if( ((iter+1) % update_rate) == 0 )
        {
            print( "%d: %g \t  %g  %g\n", iter+1, fit1 + fit2, fit1, fit2 );
        }

        if( iter+1 == next_change )
        {
            update_rate *= 10;
            next_change *= 100;
        }
    }

    delete_polygon_point_neighbours( n_point_neighbours, point_neighbours,
                                     NULL );

    for_less( point, 0, n_points )
    {
        FREE( weights[point] );
        FREE( weighted_points[point] );
    }

    for_less( point, 0, polygons->n_points )
        FREE( distances[point] );
    FREE( distances );

    FREE( n_weights );
    FREE( weights );
    FREE( weighted_points );
}
