#include  <internal_volume_io.h>
#include  <bicpl.h>

typedef  Real  LSQ_TYPE;

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters,
    Real             step );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename, initial_filename;
    int                  n_objects, n_i_objects, n_iters;
    File_formats         format;
    object_struct        **object_list, **i_object_list;
    polygons_struct      *polygons;
    Point                *init_points;
    Real                 step;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj [n_iters]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_iters );
    (void) get_real_argument( 1.0, &step );

    if( get_string_argument( NULL, &initial_filename ) )
    {
        if( input_graphics_file( initial_filename, &format, &n_i_objects,
                                 &i_object_list ) != OK || n_i_objects != 1 ||
            get_object_type(i_object_list[0]) != POLYGONS )
            return( 1 );

        polygons = get_polygons_ptr( i_object_list[0] );
        init_points = polygons->points;
        ALLOC( polygons->points, 1 );
        delete_object_list( n_i_objects, i_object_list );
    }
    else
    {
        init_points = NULL;
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    flatten_polygons( polygons, init_points, n_iters, step );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

private  Real  evaluate_fit(
    int         n_equations,
    int         n_nodes_per_equation[],
    int         *node_list[],
    LSQ_TYPE    constants[],
    LSQ_TYPE    *node_weights[],
    Real        node_values[] )
{
    int   eq, n;
    Real  fit, diff;

    fit = 0.0;

    for_less( eq, 0, n_equations )
    {
        diff = (Real) constants[eq];
        for_less( n, 0, n_nodes_per_equation[eq] )
        {
            diff += (Real) node_weights[eq][n] *
                    (Real) node_values[node_list[eq][n]];
        }

        fit += diff * diff;
    }

    return( fit );
}

private  void  evaluate_fit_derivative(
    int         n_equations,
    int         n_nodes_per_equation[],
    int         *node_list[],
    LSQ_TYPE    constants[],
    LSQ_TYPE    *node_weights[],
    int         n_nodes,
    Real        node_values[],
    Real        derivatives[] )
{
    int   p, n, eq;
    Real  diff;

    for_less( p, 0, n_nodes )
        derivatives[p] = 0.0;

    for_less( eq, 0, n_equations )
    {
        diff = (Real) constants[eq];
        for_less( n, 0, n_nodes_per_equation[eq] )
        {
            diff += (Real) node_weights[eq][n] *
                    (Real) node_values[node_list[eq][n]];
        }
        diff *= 2.0;

        for_less( n, 0, n_nodes_per_equation[eq] )
            derivatives[node_list[eq][n]] += diff * (Real) node_weights[eq][n];
    }
}

private  void  evaluate_fit_along_line(
    int          n_equations,
    int          n_nodes_per_equation[],
    int          *node_list[],
    LSQ_TYPE    constants[],
    LSQ_TYPE    *node_weights[],
    int         n_nodes,
    Real        node_values[],
    Real        line_coefs[],
    Real        *a_ptr,
    Real        *b_ptr )
{
    int   node, n, eq;
    Real  weight, constant, linear, a, b;

    a = 0.0;
    b = 0.0;

    for_less( eq, 0, n_equations )
    {
        linear = 0.0;
        constant = (Real) constants[eq];
        for_less( n, 0, n_nodes_per_equation[eq] )
        {
            weight = (Real) node_weights[eq][n];
            node = node_list[eq][n];
            constant += weight * node_values[node];
            linear += weight * line_coefs[node];
        }

        b += constant * linear;
        a += linear * linear;
    }

    *a_ptr = a;
    *b_ptr = 2.0 * b;
}

private  void  minimize_along_line(
    int         n_equations,
    int         n_nodes_per_equation[],
    int         *node_list[],
    LSQ_TYPE    constants[],
    LSQ_TYPE    *node_weights[],
    int         n_nodes,
    Real        node_values[],
    Real        line_coefs[] )
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

private  Real   local_minimize_lsq(
    int              n_parameters,
    int              n_equations,
    int              n_nodes_per_equation[],
    int              *node_list[],
    LSQ_TYPE         constants[],
    LSQ_TYPE         *node_weights[],
    int              n_iters,
    Real             step,
    Real             node_values[] )
{
    Real              fit, len, test_fit;
    int               iter, p;
    int               update_rate;
    Real              *derivs, *test_values;
    Real              last_update_time, current_time;

    ALLOC( derivs, n_parameters );
    ALLOC( test_values, n_parameters );

    fit = evaluate_fit( n_equations, n_nodes_per_equation,
                        node_list, constants, node_weights, node_values );

    print( "Initial  %g\n", fit );
    (void) flush_file( stdout );

    update_rate = 1;
    last_update_time = current_cpu_seconds();

    for_less( iter, 0, n_iters )
    {
        evaluate_fit_derivative( n_equations,
                                 n_nodes_per_equation,
                                 node_list, constants, node_weights,
                                 n_parameters, node_values, derivs );

        len = step;
        while( len > 1.0e-20 )
        {
            for_less( p, 0, n_parameters )
                test_values[p] = node_values[p] - len * derivs[p];

            test_fit =  evaluate_fit( n_equations,
                             n_nodes_per_equation,
                             node_list, constants, node_weights, test_values );

            if( test_fit < fit )
            {
                for_less( p, 0, n_parameters )
                    node_values[p] = test_values[p];
                fit = test_fit;
                break;
            }

            len /= 2.0;
        }

        if( ((iter+1) % update_rate) == 0 || iter == n_iters - 1 )
        {
            print( "%d: %g\n", iter+1, fit );
            (void) flush_file( stdout );
            current_time = current_cpu_seconds();
            if( current_time - last_update_time < 1.0 )
                update_rate *= 10;
            last_update_time = current_time;
        }
    }

    FREE( derivs );
    FREE( test_values );

    return( fit );
}
private  int  create_coefficients(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              **neighbours,
    Smallest_int     interior_flags[],
    int              n_fixed,
    int              fixed_indices[],
    Real             *fixed_pos[2],
    int              to_parameters[],
    int              to_fixed_index[],
    int              *n_nodes_involved[],
    int              **node_list[],
    Real             *constants[],
    Real             **node_weights[] )
{
    int              n_equations, node, p, eq, dim, n_nodes_in;
    int              neigh;
    Point            neigh_points[MAX_POINTS_PER_POLYGON];
    Real             flat[2][MAX_POINTS_PER_POLYGON];
    Real             *weights[2][2], cons[2];
    BOOLEAN          found, ignoring;
    progress_struct  progress;

    ALLOC( weights[0][0], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[0][1], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[1][0], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[1][1], MAX_POINTS_PER_POLYGON );

    n_equations = 0;
    for_less( node, 0, polygons->n_points )
    {
        found = to_parameters[node] >= 0;
        for_less( p, 0, n_neighbours[node] )
        {
            if( to_parameters[neighbours[node][p]] >= 0 )
            {
                found = TRUE;
                break;
            }
        }
        if( found && n_neighbours[node] >= 2 )
            ++n_equations;
    }

    n_equations *= 2;

    ALLOC( *n_nodes_involved, n_equations );
    ALLOC( *constants, n_equations );
    ALLOC( *node_weights, n_equations );
    ALLOC( *node_list, n_equations );

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Creating coefficients" );
    eq = 0;
    for_less( node, 0, polygons->n_points )
    {
        found = to_parameters[node] >= 0;
        for_less( p, 0, n_neighbours[node] )
        {
            if( to_parameters[neighbours[node][p]] >= 0 )
            {
                found = TRUE;
                break;
            }
        }

        if( !found || n_neighbours[node] < 2 )
            continue;

        for_less( p, 0, n_neighbours[node] )
            neigh_points[p] = polygons->points[neighbours[node][p]];

        flatten_around_vertex( &polygons->points[node],
                               n_neighbours[node], neigh_points,
                               (BOOLEAN) interior_flags[node],
                               flat[0], flat[1] );

        ignoring = FALSE;
        if( !get_prediction_weights_2d( 0.0, 0.0, n_neighbours[node],
                                         flat[0], flat[1],
                                         weights[0], &cons[0],
                                         weights[1], &cons[1] ) )
        {
            print_error( "Error in interpolation weights, ignoring..\n" );
            ignoring = TRUE;
        }

        if( ignoring )
        {
            (*n_nodes_involved)[eq] = 0;
            (*constants)[eq] = 0.0;
            ++eq;
            (*n_nodes_involved)[eq] = 0;
            (*constants)[eq] = 0.0;
            ++eq;
            continue;
        }

        for_less( dim, 0, 2 )
        {
            n_nodes_in = 0;
            if( to_parameters[node] >= 0 )
                ++n_nodes_in;

            for_less( p, 0, n_neighbours[node] )
            {
                if( to_parameters[neighbours[node][p]] >= 0 )
                    n_nodes_in += 2;
            }

            (*n_nodes_involved)[eq] = n_nodes_in;
            ALLOC( (*node_list)[eq], (*n_nodes_involved)[eq] );
            ALLOC( (*node_weights)[eq], (*n_nodes_involved)[eq] );

            n_nodes_in = 0;
            if( to_parameters[node] >= 0 )
            {
                (*node_list)[eq][0] = 2 * to_parameters[node] + dim;
                (*node_weights)[eq][0] = 1.0;
                ++n_nodes_in;
                (*constants)[eq] = 0.0;
            }
            else
                (*constants)[eq] = fixed_pos[dim][to_fixed_index[node]];

            (*constants)[eq] -= cons[dim];

            for_less( p, 0, n_neighbours[node] )
            {
                neigh = neighbours[node][p];
                if( to_parameters[neigh] >= 0 )
                {
                    (*node_list)[eq][n_nodes_in] = 2 * to_parameters[neigh] + 0;
                    (*node_weights)[eq][n_nodes_in] = -weights[dim][0][p];
                    ++n_nodes_in;
                    (*node_list)[eq][n_nodes_in] = 2 * to_parameters[neigh] + 1;
                    (*node_weights)[eq][n_nodes_in] = -weights[dim][1][p];
                    ++n_nodes_in;
                }
                else
                {
                    (*constants)[eq] += -weights[dim][0][p] *
                                        fixed_pos[0][to_fixed_index[neigh]]
                                        -weights[dim][1][p] *
                                        fixed_pos[1][to_fixed_index[neigh]];
                }
            }

            ++eq;
        }

        update_progress_report( &progress, node + 1 );
    }

    terminate_progress_report( &progress );

#ifdef DEBUG
#define DEBUG
#undef DEBUG
    for_less( eq, 0, n_equations )
    {
        print( "%3d: %g : ", eq, (*constants)[eq] );
        for_less( p, 0, (*n_nodes_involved)[eq] )
        {
            print( " %d:%g ", (*node_list)[eq][p], (*node_weights)[eq][p] );
        }
        print( "\n" );
    }
#endif

    FREE( weights[0][0] );
    FREE( weights[0][1] );
    FREE( weights[1][0] );
    FREE( weights[1][1] );

    return( n_equations );
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters,
    Real             step )
{
    int              i, point, *n_neighbours, **neighbours, n, neigh, which;
    int              n_equations, *n_nodes_per_equation, **node_list;
    int              n_fixed, *fixed_indices;
    Real             *fixed_pos[2], scale, dist1, dist2;
    Real             *constants, **node_weights, sum_xx, sum_xy;
    Point            neigh_points[MAX_POINTS_PER_POLYGON], *new_points;
    Real             *parameters;
    int              *to_parameters, *to_fixed_index, ind;
    Vector           x_dir, y_dir, offset;
    Smallest_int     *interior_flags;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, &interior_flags, NULL );

    for_less( which, 0, polygons->n_points )
    {
        if( n_neighbours[which] > 1 )
            break;
    }

    n_fixed = 1 + n_neighbours[which];
    ALLOC( fixed_indices, n_fixed );
    ALLOC( fixed_pos[0], n_fixed );
    ALLOC( fixed_pos[1], n_fixed );

    fixed_indices[0] = which;
    for_less( i, 0, n_neighbours[which] )
        fixed_indices[i+1] = neighbours[which][i];

    for_less( i, 0, n_neighbours[which] )
        neigh_points[i] = polygons->points[neighbours[which][i]];

    fixed_pos[0][0] = 0.0;
    fixed_pos[1][0] = 0.0;

    flatten_around_vertex( &polygons->points[which], n_neighbours[which],
                           neigh_points, (BOOLEAN) interior_flags[which],
                           &fixed_pos[0][1], &fixed_pos[1][1] );

    n_fixed = 3;

    ALLOC( to_parameters, polygons->n_points );
    ALLOC( to_fixed_index, polygons->n_points );
    ind = 0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( i, 0, n_fixed )
        {
            if( point == fixed_indices[i] )
                break;
        }
        if( i < n_fixed )
        {
            to_fixed_index[point] = i;
            to_parameters[point] = -1;
        }
        else
        {
            to_fixed_index[point] = -1;
            to_parameters[point] = ind;
            ++ind;
        }
    }


    n_equations = create_coefficients( polygons,
                                       n_neighbours, neighbours, interior_flags,
                                       n_fixed, fixed_indices, fixed_pos,
                                       to_parameters, to_fixed_index,
                                       &n_nodes_per_equation,
                                       &node_list, &constants, &node_weights );
    ALLOC( parameters, 2 * (polygons->n_points - n_fixed) );

    if( init_points == NULL )
        init_points = polygons->points;

    SUB_POINTS( x_dir, init_points[neighbours[which][0]],
                init_points[which] );
    NORMALIZE_VECTOR( x_dir, x_dir );
    fill_Point( y_dir, -RVector_y(x_dir), RVector_x(x_dir), 0.0 );
    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            SUB_POINTS( offset, init_points[point], init_points[which] );
            parameters[2*to_parameters[point]] =
                           DOT_VECTORS( offset, x_dir );
            parameters[2*to_parameters[point]+1] =
                           DOT_VECTORS( offset, y_dir );
        }
    }

    (void) local_minimize_lsq( 2 * (polygons->n_points - n_fixed), n_equations,
                         n_nodes_per_equation, node_list, constants,
                         node_weights, n_iters, step, parameters );

    ALLOC( new_points, polygons->n_points );

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            fill_Point( new_points[point],
                        parameters[2*to_parameters[point]],
                        parameters[2*to_parameters[point]+1], 0.0 );
        }
        else
        {
            fill_Point( new_points[point],
                        fixed_pos[0][to_fixed_index[point]],
                        fixed_pos[1][to_fixed_index[point]], 0.0 );
        }
    }

    FREE( parameters );

    sum_xx = 0.0;
    sum_xy = 0.0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            dist1 = distance_between_points( &polygons->points[point],
                                             &polygons->points[neigh] );
            dist2 = distance_between_points( &new_points[point],
                                             &new_points[neigh] );

            sum_xx += dist2 * dist2;
            sum_xy += dist1 * dist2;
        }
    }

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     interior_flags, NULL );

    if( sum_xx == 0.0 )
    {
        print_error( "sum_xx = 0.0" );
        scale = 1.0;
    }
    else
    {
        scale = sum_xy / sum_xx;
    }

    scale = 1.0;
    for_less( point, 0, polygons->n_points )
    {
        SCALE_POINT( polygons->points[point], new_points[point], scale );
    }

    FREE( new_points );
    FREE( to_parameters );
    FREE( to_fixed_index );
    FREE( fixed_pos[0] );
    FREE( fixed_pos[1] );
}
