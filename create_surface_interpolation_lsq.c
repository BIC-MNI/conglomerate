#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_SMOOTHNESS   1.0

typedef  Real  ftype;

private  void   create_surface_interpolation(
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

    create_surface_interpolation( objects[0], n_points, points, values,
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

private  Real  evaluate_fit(
    int     n_equations,
    int     n_nodes_per_equation[],
    int     *node_list[],
    ftype   constants[],
    ftype   *node_weights[],
    Real    node_values[] )
{
    int   eq, n;
    Real  fit, diff;

    fit = 0.0;

    for_less( eq, 0, n_equations )
    {
        diff = constants[eq];
        for_less( n, 0, n_nodes_per_equation[eq] )
            diff += node_weights[eq][n] * node_values[node_list[eq][n]];

        fit += diff * diff;
    }

    return( fit );
}

private  void  evaluate_fit_along_line(
    int     n_equations,
    int     n_nodes_per_equation[],
    int     *node_list[],
    ftype   constants[],
    ftype   *node_weights[],
    int     n_nodes,
    Real    node_values[],
    Real    line_coefs[],
    Real    *a_ptr,
    Real    *b_ptr )
{
    int   node, n, eq;
    Real  weight, constant, linear, a, b;

    a = 0.0;
    b = 0.0;

    for_less( eq, 0, n_equations )
    {
        linear = 0.0;
        constant = constants[eq];
        for_less( n, 0, n_nodes_per_equation[eq] )
        {
            weight = node_weights[eq][n];
            node = node_list[eq][n];
            constant += weight * node_values[node];
            linear += weight * line_coefs[node];
        }

        b += 2.0 * constant * linear;
        a += linear * linear;
    }

    *a_ptr = a;
    *b_ptr = b;
}

private  void  minimize_along_line(
    int     n_equations,
    int     n_nodes_per_equation[],
    int     *node_list[],
    ftype   constants[],
    ftype   *node_weights[],
    int     n_nodes,
    Real    node_values[],
    Real    line_coefs[] )
{
    int   node;
    Real  a, b, t;

    evaluate_fit_along_line( n_equations, n_nodes_per_equation,
                             node_list, constants, node_weights,
                             n_nodes, node_values, line_coefs, &a, &b );

    if( a == 0.0 )
        return;

    t = -b / (2.0 * a);

    for_less( node, 0, n_nodes )
        node_values[node] += t * line_coefs[node];
}

private  void  evaluate_fit_derivative(
    int     n_equations,
    int     n_nodes_per_equation[],
    int     *node_list[],
    ftype   constants[],
    ftype   *node_weights[],
    int     n_nodes,
    Real    node_values[],
    Real    derivatives[] )
{
    int   node, p, n, eq;
    Real  diff;

    for_less( p, 0, n_nodes )
        derivatives[p] = 0.0;

    for_less( eq, 0, n_equations )
    {
        diff = constants[eq];
        for_less( n, 0, n_nodes_per_equation[eq] )
            diff += node_weights[eq][n] * node_values[node_list[eq][n]];
        diff *= 2.0;

        for_less( n, 0, n_nodes_per_equation[eq] )
        {
            node = node_list[eq][n];
            derivatives[node] += diff * node_weights[eq][n];
        }
    }
}

#ifdef OLD
private  void  fast_minimize_nodes(
    int     node,
    int     n_equations,
    int     n_nodes_per_equation[],
    int     *node_list[],
    ftype   constants[],
    ftype   *node_weights[],
    int     n_nodes,
    Real    node_values[] )
{
    int            eq, i;
    Real           linear, constant, a, b;

    a = 0.0;
    b = 0.0;

    for_less( eq, 0, n_equations )
    {
        for_less( i, 0, n_nodes_per_equation[eq] )
        {
            if( node_list[eq][i] == node )
                break;
        }

        if( i >= n_nodes_per_equation[eq] )
            continue;

        linear = 0.0;
        constant = constants[eq];
        for_less( i, 0, n_nodes_per_equation[eq] )
        {
            if( node_list[eq][i] == node )
                linear += node_weights[eq][i];
            else
                constant += node_weights[eq][i] * node_values[node_list[eq][i]];
        }

        a += linear * linear;
        b += 2.0 * linear * constant;
    }

    if( a == 0.0 )
        print_error( "Error getting least squares.\n" );

    node_values[node] = -b / (2.0*a);
}
#endif

#define  INCREMENT  1.0e-4

private  void  minimize_cost(
    int     n_equations,
    int     n_nodes_per_equation[],
    int     *node_list[],
    ftype   constants[],
    ftype   *node_weights[],
    int     n_nodes,
    Real    node_values[] )
{
    int     n, iter, n_iters;
    Real    *next_values, *derivs;

    ALLOC( derivs, n_nodes );
    ALLOC( next_values, n_nodes );
    for_less( n, 0, n_nodes )
        next_values[n] = node_values[n];

    if( getenv( "N_ITERS" ) == NULL ||
        sscanf( getenv("N_ITERS"), "%d", &n_iters ) != 1 )
        n_iters = 50;

    for_less( iter, 0, n_iters )
    {
        evaluate_fit_derivative( n_equations, n_nodes_per_equation,
                                 node_list, constants, node_weights,
                                 n_nodes, next_values, derivs );

        minimize_along_line( n_equations, n_nodes_per_equation,
                             node_list, constants, node_weights,
                             n_nodes, next_values, derivs );

        for_less( n, 0, n_nodes )
        {
            derivs[n] = next_values[n] - node_values[n];
            next_values[n] = node_values[n];
        }

        minimize_along_line( n_equations, n_nodes_per_equation,
                             node_list, constants, node_weights,
                             n_nodes, next_values, derivs );
    }

    for_less( n, 0, n_nodes )
        node_values[n] = next_values[n];

    FREE( next_values );
    FREE( derivs );
}

private  void  output_surface(
    STRING  filename,
    int     p0,
    int     p1,
    int     n_equations,
    int     n_nodes_per_equation[],
    int     *node_list[],
    ftype   constants[],
    ftype   *node_weights[],
    Real    node_values[] )
{
    FILE             *file;
    int              i, j, size, ind;
    Real             x, y, save0, save1, scale, fit, *q_values;
    Point            point;
    Vector           normal;
    quadmesh_struct  *quadmesh;
    object_struct    *object;

    if( getenv( "SIZE" ) == NULL ||
        sscanf( getenv("SIZE"), "%d", &size ) != 1 )
        return;

    object = create_object( QUADMESH );
    quadmesh = get_quadmesh_ptr( object );

    initialize_quadmesh( quadmesh, WHITE, NULL, size, size );

    save0 = node_values[p0];
    save1 = node_values[p1];

    if( getenv( "SCALE" ) == NULL ||
        sscanf( getenv("SCALE"), "%lf", &scale ) != 1 )
        scale = 1.0;

    fill_Vector( normal, 1.0, 1.0, 1.0 );

    ALLOC( q_values, size * size );

    for_less( i, 0, size )
    {
        x = INTERPOLATE( (Real) i / (Real) (size-1), save0 - 1.0, save0 + 1.0 );
        node_values[p0] = x;
        for_less( j, 0, size )
        {
            y = INTERPOLATE( (Real) j / (Real) (size-1), save1 - 1.0,
                             save1 + 1.0 );
            node_values[p1] = y;

            fit = evaluate_fit( n_equations, n_nodes_per_equation,
                                node_list, constants, node_weights,
                                node_values );

            ind = IJ( i, j, quadmesh->n );
            q_values[ind] = fit;

            fit *= scale;

            fill_Point( point, x, y, fit );
            set_quadmesh_point( quadmesh, i, j, &point, &normal );
        }
    }

    (void) open_file( "quadmesh.values", WRITE_FILE, ASCII_FORMAT, &file );
    for_less( ind, 0, size * size )
    {
        (void) output_real( file, q_values[ind] );
        (void) output_newline( file );
    }
    close_file( file );

    FREE( q_values );

    compute_quadmesh_normals( quadmesh );

    node_values[p0] = save0;
    node_values[p1] = save1;

    (void) output_graphics_file( filename, BINARY_FORMAT, 1, &object ) ;

    delete_object( object );
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
    int              *n_nodes_involved[],
    int              **node_list[],
    ftype            *constants[],
    ftype            **node_weights[] )
{
    int              i, n_equations, eq, poly, node, p, size;
    polygons_struct  *polygons;
    Point            polygon_points[MAX_POINTS_PER_POLYGON];
    Point            neigh_points[MAX_POINTS_PER_POLYGON];
    Real             weights[MAX_POINTS_PER_POLYGON], dist, avg_dist, weight;
    Real             x_flat[MAX_POINTS_PER_POLYGON];
    Real             y_flat[MAX_POINTS_PER_POLYGON];
    ftype            consistency_weights[MAX_POINTS_PER_POLYGON];
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

        (*constants)[eq] = (ftype) (- values[p] * interp_weight);

        get_polygon_interpolation_weights( &point_on_surface, size,
                                           polygon_points, weights );

        for_less( i, 0, size )
        {
            (*node_list)[eq][i] = polygons->indices[POINT_INDEX(
                                         polygons->end_indices,poly,i)];
            (*node_weights)[eq][i] = (ftype) (weights[i] * interp_weight);
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

        (*constants)[eq] = (ftype) 0.0;
        (*n_nodes_involved)[eq] = 1 + n_neighbours[node];
        ALLOC( (*node_list)[eq], (*n_nodes_involved)[eq] );
        ALLOC( (*node_weights)[eq], (*n_nodes_involved)[eq] );

        (*node_list)[eq][0] = node;
        (*node_weights)[eq][0] = weight;

#define FLATTEN
#ifdef  FLATTEN
        flatten_around_vertex( &polygons->points[node],
                               n_neighbours[node],
                               neigh_points, x_flat, y_flat );

        if( !get_interpolation_weights_2d( 0.0, 0.0, n_neighbours[node],
                                           x_flat, y_flat,
                                           consistency_weights ) )
        {
            print_error( "Error in interpolation weights, using avg..\n" );

            for_less( p, 0, n_neighbours[node] )
            {
                consistency_weights[p] = (ftype) 1.0 /
                                          (ftype) n_neighbours[node];
            }
        }
#else
        for_less( p, 0, n_neighbours[node] )
            consistency_weights[p] = (ftype) 1.0 / (ftype) n_neighbours[node];
#endif

        for_less( p, 0, n_neighbours[node] )
        {
            (*node_list)[eq][1+p] = neighbours[node][p];
            (*node_weights)[eq][1+p] = (ftype) -weight * consistency_weights[p];
        }

        ++eq;
    }

    return( n_equations );
}

private  void   create_surface_interpolation(
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
    int               iter, neigh, n_edges;
    int               next_change, update_rate;
    int               n_equations, *n_nodes_per_equation, **node_list;
    ftype             **node_weights, *constants;

    polygons = get_polygons_ptr( object );

    create_polygons_bintree( polygons,
                             (int) ((Real) polygons->n_items * 0.3) );

    create_polygon_point_neighbours( polygons, FALSE, &n_point_neighbours,
                                     &point_neighbours, NULL );

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
                                       &n_nodes_per_equation,
                                       &node_list, &constants, &node_weights );

    output_surface( "hyper.obj", node_list[0][0], node_list[0][1],
                    n_equations, n_nodes_per_equation,
                    node_list, constants, node_weights, node_values );

    fit = evaluate_fit( n_equations, n_nodes_per_equation,
                         node_list, constants, node_weights, node_values );

    print( "Initial  %g\n", fit );

    update_rate = 1;
    next_change = 10;
    for_less( iter, 0, n_iters )
    {
        minimize_cost( n_equations, n_nodes_per_equation,
                       node_list, constants, node_weights,
                       polygons->n_points, node_values );

        fit =  evaluate_fit( n_equations, n_nodes_per_equation,
                             node_list, constants, node_weights, node_values );

        if( ((iter+1) % update_rate) == 0 )
        {
            print( "%d: %g\n", iter+1, fit );
        }

        if( iter+1 == next_change )
        {
            update_rate *= 10;
            next_change *= 100;
        }
    }

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
                                     point_neighbours, NULL );
}
