#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_SMOOTHNESS   1.0

private  void   create_surface_interpolation(
    object_struct    *object,
    int              n_points,
    Point            points[],
    Real             values[],
    Real             smoothness,
    int              n_iters,
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
    STRING               output_filename;
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

    create_surface_interpolation( objects[0], n_points, points, values,
                                  smoothness, n_iters, node_values );

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
    int    *n_neighbours,
    int    *neighbours[],
    Real   interp_weight,
    Real   smooth_weight,
    int    n_interp_points,
    int    n_weights[],
    Real   **weights,
    int    **weighted_points,
    Real   values[],
    Real   total_length,
    int    n_nodes,
    Point  nodes[],
    Real   node_values[],
    Real   *fit1,
    Real   *fit2 )
{
    int   node, p, i;
    Real  val, dist, diff, f1, f2;

    f1 = 0.0;

    for_less( p, 0, n_interp_points )
    {
        val = 0.0;
        for_less( i, 0, n_weights[p] )
            val += weights[p][i] * node_values[weighted_points[p][i]];

        f1 += (val - values[p]) * (val - values[p]);
    }

    f1 *= interp_weight;

    f2 = 0.0;

    for_less( node, 0, n_nodes )
    {
        for_less( p, 0, n_neighbours[node] )
        {
            dist = distance_between_points( &nodes[node],
                                            &nodes[neighbours[node][p]] );
            diff = node_values[node] - node_values[neighbours[node][p]];

            diff /= (dist / total_length);

            f2 += smooth_weight * diff * diff;
        }
    }

    if( fit1 != NULL ) *fit1 = f1;
    if( fit2 != NULL ) *fit2 = f2;

    return( f1 + f2 );
}

private  void  minimize_node(
    int    node,
    int    n_neighbours,
    int    neighbours[],
    Real   interp_weight,
    Real   smooth_weight,
    int    n_interp_points,
    int    n_weights[],
    Real   **weights,
    int    **weighted_points,
    Real   values[],
    Real   total_length,
    int    n_nodes,
    Point  nodes[],
    Real   node_values[] )
{
    int   p, i, which;
    Real  a, b, a1, b1, a2, b2, val, linear, constant, dist;

    a1 = 0.0;
    b1 = 0.0;

    for_less( p, 0, n_interp_points )
    {
        for_less( which, 0, n_weights[p] )
        {
            if( weighted_points[p][which] == node )
                break;
        }

        if( which >= n_weights[p] )
            continue;

        val = -values[p];
        for_less( i, 0, n_weights[p] )
            val += weights[p][i] * node_values[weighted_points[p][i]];

        linear = weights[p][which];
        constant = val - linear * node_values[node];

        a1 += linear * linear;
        b1 += 2.0 * constant * linear;
    }

    a1 *= interp_weight;
    b1 *= interp_weight;

    a2 = 0.0;
    b2 = 0.0;
    for_less( p, 0, n_neighbours )
    {
        dist = distance_between_points( &nodes[node], &nodes[neighbours[p]] );
        linear = 1.0 / (dist / total_length);
        constant = -node_values[neighbours[p]] / (dist / total_length);
        a2 += smooth_weight * linear * linear;
        b2 += smooth_weight * 2.0 * constant * linear;
    }

    a = a1 + a2;
    b = b1 + b2;
    if( a == 0.0 )
    {
        print_error( "a == 0.0\n" );
    }
    else
    {
        node_values[node] = - b / (2.0 * a);
    }
}

#define  INCREMENT  1.0e-4

private  void  minimize_cost(
    Real   interp_weight,
    Real   smooth_weight,
    int    n_interp_points,
    int    n_weights[],
    Real   **weights,
    int    **weighted_points,
    Real   values[],
    Real   total_length,
    int    n_nodes,
    Point  nodes[],
    Real   node_values[],
    int    n_neighbours[],
    int    *neighbours[] )
{
    int     node;
    Real    fit0, fit1, fit2, fit3, save, fa, fb;

    for_less( node, 0, n_nodes )
    {
/*
        fit0 = evaluate_fit( n_neighbours, neighbours,
                             interp_weight, smooth_weight, n_interp_points,
                             n_weights, weights, weighted_points, values,
                             total_length, n_nodes, nodes,
                             node_values, NULL, NULL );
*/

        minimize_node( node, n_neighbours[node], neighbours[node],
                       interp_weight, smooth_weight,
                       n_interp_points, n_weights, weights, weighted_points,
                       values, total_length, n_nodes, nodes, node_values );

/*
        save = node_values[node];

        fit2 = evaluate_fit( n_neighbours, neighbours,
                             interp_weight, smooth_weight, n_interp_points,
                             n_weights, weights, weighted_points, values,
                             total_length, n_nodes, nodes,
                             node_values, &fa, &fb );

        node_values[node] = save - INCREMENT;
        fit1 = evaluate_fit( n_neighbours, neighbours,
                             interp_weight, smooth_weight, n_interp_points,
                             n_weights, weights, weighted_points, values,
                             total_length, n_nodes, nodes,
                             node_values, NULL, NULL );

        node_values[node] = save + INCREMENT;
        fit3 = evaluate_fit( n_neighbours, neighbours,
                             interp_weight, smooth_weight, n_interp_points,
                             n_weights, weights, weighted_points, values,
                             total_length, n_nodes, nodes,
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
    int              n_iters,
    Real             node_values[] )
{
    polygons_struct   *polygons;
    Real              total_length, sum_x, sum_xx, std_dev;
    Point             polygon_points[MAX_POINTS_PER_POLYGON];
    Point             point_on_surface;
    Real              **weights;
    Real              dist, interp_weight, smooth_weight, fit1, fit2;
    int               point, *n_point_neighbours, **point_neighbours, p;
    int               iter, size, obj_index, neigh;
    int               **weighted_points, *n_weights;

    polygons = get_polygons_ptr( object );

    create_polygons_bintree( polygons,
                             (int) ((Real) polygons->n_items * 0.3) );

    create_polygon_point_neighbours( polygons, &n_point_neighbours,
                                     &point_neighbours, NULL );

    total_length = 0.0;

    for_less( point, 0, polygons->n_points )
    {
        for_less( neigh, 0, n_point_neighbours[point] )
            total_length += distance_between_points( &polygons->points[point],
                          &polygons->points[point_neighbours[point][neigh]] );
    }

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

    for_less( point, 0, polygons->n_points )
    {
        node_values[point] = 1.0 + sum_x / (Real) n_points;
    }

    interp_weight = 1.0 / (Real) n_points / std_dev;
    smooth_weight = smoothness;

    (void) evaluate_fit( n_point_neighbours, point_neighbours,
                         interp_weight, smooth_weight, n_points,
                         n_weights, weights, weighted_points, values,
                         total_length, polygons->n_points, polygons->points,
                         node_values, &fit1, &fit2 );

    print( "Initial  %g %g\n", fit1, fit2 );

    for_less( iter, 0, n_iters )
    {
        minimize_cost( interp_weight, smooth_weight,
                       n_points, n_weights, weights, weighted_points, values,
                       total_length,
                       polygons->n_points, polygons->points, node_values,
                       n_point_neighbours, point_neighbours );

        (void) evaluate_fit( n_point_neighbours, point_neighbours,
                      interp_weight, smooth_weight, n_points,
                      n_weights, weights, weighted_points, values,
                      total_length, polygons->n_points, polygons->points,
                      node_values, &fit1, &fit2 );

        print( "%d: %g \t  %g  %g\n", iter+1, fit1 + fit2, fit1, fit2 );
    }

    delete_polygon_point_neighbours( n_point_neighbours, point_neighbours,
                                     NULL );

    for_less( point, 0, n_points )
    {
        FREE( weights[point] );
        FREE( weighted_points[point] );
    }
    FREE( n_weights );
    FREE( weights );
    FREE( weighted_points );
}
