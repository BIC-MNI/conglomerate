#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR 0.4

private  void  reparameterize(
    polygons_struct   *original,
    object_struct     *initial,
    polygons_struct   *model,
    Real              scale,
    Real              tolerance,
    int               n_iters );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename;
    STRING               model_filename, initial_filename;
    File_formats         init_format, src_format, model_format;
    int                  n_init_objects, n_src_objects, n_model_objects;
    object_struct        **src_objects, **model_objects, **init_objects;
    polygons_struct      *original, *model, *initial;
    Real                 tolerance, ratio, scale;
    int                  n_iters, method, grid_size;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &initial_filename ) ||
        !get_string_argument( NULL, &model_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  input.obj  model.obj output.obj\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1, &n_iters );
    (void) get_real_argument( 0.0, &tolerance );
    (void) get_real_argument( 0.0, &scale );

    if( input_graphics_file( input_filename, &src_format,
                             &n_src_objects, &src_objects ) != OK ||
        n_src_objects != 1 || get_object_type(src_objects[0]) != POLYGONS )
    {
        print_error( "Error in %s\n", input_filename );
        return( 1 );
    }

    if( input_graphics_file( initial_filename, &init_format,
                             &n_init_objects, &init_objects ) != OK ||
        n_init_objects != 1 || get_object_type(init_objects[0]) != POLYGONS )
    {
        print_error( "Error in %s\n", initial_filename );
        return( 1 );
    }

    if( input_graphics_file( model_filename, &model_format,
                             &n_model_objects, &model_objects ) != OK ||
        n_model_objects != 1 || get_object_type(model_objects[0]) != POLYGONS )
    {
        print_error( "Error in %s\n", model_filename );
        return( 1 );
    }

    model = get_polygons_ptr( model_objects[0] );
    original = get_polygons_ptr( src_objects[0] );
    initial = get_polygons_ptr( init_objects[0] );

    if( !objects_are_same_topology( original->n_points,
                                    original->n_items,
                                    original->end_indices,
                                    original->indices,
                                    model->n_points,
                                    model->n_items,
                                    model->end_indices,
                                    model->indices ) ||
        !objects_are_same_topology( original->n_points,
                                    original->n_items,
                                    original->end_indices,
                                    original->indices,
                                    initial->n_points,
                                    initial->n_items,
                                    initial->end_indices,
                                    initial->indices ) )
    {
        print_error( "Mismatched topology.\n" );
        return( 1 );
    }

    reparameterize( original, init_objects[0], model, scale,
                    tolerance, n_iters );

    if( output_graphics_file( output_filename, src_format, n_src_objects,
                              src_objects ) != OK )
        return( 1 );

    delete_object_list( n_src_objects, src_objects );

    output_alloc_to_file( NULL );

    return( 0 );
}

#ifdef OLD
private  Real  dot_vectors(
    Real  v1[],
    Real  v2[] )
{
    return( v1[X] * v2[X] + v1[Y] * v2[Y] + v1[Z] * v2[Z] );
}

private  void  sub_vectors(
    Real  v1[],
    Real  v2[],
    Real  sub[] )
{
    sub[X] = v1[X] - v2[X];
    sub[Y] = v1[Y] - v2[Y];
    sub[Z] = v1[Z] - v2[Z];
}

private  void  cross_vectors(
    Real  v1[],
    Real  v2[],
    Real  c[] )
{
    c[X] = v1[Y] * v2[Z] - v1[Z] * v2[Y];
    c[Y] = v1[Z] * v2[X] - v1[X] * v2[Z];
    c[Z] = v1[X] * v2[Y] - v1[Y] * v2[X];
}

private  Real  evaluate_fit(
    Real    point[],
    int     n_neighbours,
    Real    (*neighbours)[N_DIMENSIONS],
    Real    lengths[],
    Real    normal[] )
{
    int    n, next_n;
    Real   v1[N_DIMENSIONS], v2[N_DIMENSIONS];
    Real   test_normal[N_DIMENSIONS];
    Real   fit, dist, diff, dx, dy, dz, len_t, len_n;

    fit = 0.0;

    len_n = dot_vectors( normal, normal );

    for_less( n, 0, n_neighbours )
    {
        next_n = (n + 1) % n_neighbours;

        sub_vectors( neighbours[n], point, v1 );
        sub_vectors( neighbours[next_n], point, v2 );
        cross_vectors( v1, v2, test_normal );
        len_t = dot_vectors( test_normal, test_normal );
        if( dot_vectors( test_normal, normal ) / sqrt( len_n * len_t ) <= 0.1 )
            return( 1.0e30 );
    }

    for_less( n, 0, n_neighbours )
    {
        dx = point[X] - neighbours[n][X];
        dy = point[Y] - neighbours[n][Y];
        dz = point[Z] - neighbours[n][Z];
        dist = dx * dx + dy * dy + dz * dz;
        if( dist > 0.0 )
            dist = sqrt( dist );

        diff = (dist - lengths[n]) / lengths[n];
        fit += diff * diff;
    }

    return( fit );
}

private  void  get_edge_deriv(
    Real    point[],
    Real    length,
    Real    neighbour[],
    Real    deriv[] )
{
    Real   dist, diff, dx, dy, dz, factor;

    dx = point[X] - neighbour[X];
    dy = point[Y] - neighbour[Y];
    dz = point[Z] - neighbour[Z];
    dist = dx * dx + dy * dy + dz * dz;
    if( dist > 0.0 )
        dist = sqrt( dist );

    diff = dist - length;

    factor = 2.0 * diff / dist / length / length;

    deriv[X] = factor * dx;
    deriv[Y] = factor * dy;
    deriv[Z] = factor * dz;
}

private  void  get_plane_normal(
    int     n_points,
    Real    (*points)[N_DIMENSIONS],
    int     indices[],
    Real    normal[] )
{
    int     i, next_i;
    Real    vx, vy, vz, x, y, z, tx, ty, tz;

    vx = 0.0;
    vy = 0.0;
    vz = 0.0;

    tx = points[indices[0]][X];
    ty = points[indices[0]][Y];
    tz = points[indices[0]][Z];

    for_less( i, 0, n_points )
    {
        next_i = (i + 1) % n_points;

        x = tx;
        y = ty;
        z = tz;
        tx = points[indices[next_i]][X];
        ty = points[indices[next_i]][Y];
        tz = points[indices[next_i]][Z];

        vx -= (y + ty) * (z - tz);
        vy -= (z + tz) * (x - tx);
        vz -= (x + tx) * (y - ty);
    }

    normal[X] = vx;
    normal[Y] = vy;
    normal[Z] = vz;
}

private  void  remove_vector_component(
    Real    direction[],
    Real    plane_normal[] )
{
    Real   factor, bottom;

    bottom = dot_vectors( plane_normal, plane_normal );

    if( bottom == 0.0 )
    {
        handle_internal_error( "get_plane_normal" );
        return;
    }

    factor = dot_vectors( direction, plane_normal) / bottom;

    direction[X] -= factor * plane_normal[X];
    direction[Y] -= factor * plane_normal[Y];
    direction[Z] -= factor * plane_normal[Z];
}

private  Real  find_distance_to_edge(
    Real    origin[],
    Real    direction[],
    Real    plane_normal[],
    Real    p1[],
    Real    p2[] )
{
    Real   dist;
    Real   edge[N_DIMENSIONS];
    Real   ortho_dir[N_DIMENSIONS];
    Real   test_normal[N_DIMENSIONS];

    sub_vectors( p2, p1, edge );

    cross_vectors( direction, edge, test_normal );
    if( dot_vectors( plane_normal, test_normal ) <= 0.0 )
        return( -1.0 );

    ortho_dir[X] = direction[X];
    ortho_dir[Y] = direction[Y];
    ortho_dir[Z] = direction[Z];
    remove_vector_component( ortho_dir, edge );

    dist = dot_vectors( ortho_dir, p1 ) - dot_vectors( ortho_dir, origin );

    return( dist );
}

private  Real  find_distance_to_neighbour_edge(
    Real    origin[],
    Real    direction[],
    Real    p1[],
    Real    p2[] )
{
    Real   dist;
    Real   edge[N_DIMENSIONS];
    Real   ortho_dir[N_DIMENSIONS];

    sub_vectors( p2, p1, edge );

    ortho_dir[X] = direction[X];
    ortho_dir[Y] = direction[Y];
    ortho_dir[Z] = direction[Z];
    remove_vector_component( ortho_dir, edge );

    dist = dot_vectors( ortho_dir, p1 ) - dot_vectors( ortho_dir, origin );

    return( dist );
}

private  Real  optimize_vertex(
    polygons_struct   *model,
    Real              (*original_points)[N_DIMENSIONS],
    Real              point[],
    int               n_neighbours,
    Real              (*neighbours)[N_DIMENSIONS],
    Real              lengths[],
    int               *which_triangle,
    Real              ratio )
{
    int    n, dim, v, vertex[10000], size, exit_neighbour;
    Real   deriv[N_DIMENSIONS], edge_deriv[N_DIMENSIONS], movement, mag;
    Real   normal[N_DIMENSIONS], max_dist, dist_to_edge;
    Real   new_point[N_DIMENSIONS];
    Real   dx, dy, dz, step, len, max_dist_to_neighbour, max_step;
    Real   best_fit, new_fit, position, new_position;

    deriv[X] = 0.0;
    deriv[Y] = 0.0;
    deriv[Z] = 0.0;

    for_less( n, 0, n_neighbours )
    {
        get_edge_deriv( point, lengths[n], neighbours[n], edge_deriv );
        for_less( dim, 0, N_DIMENSIONS )
            deriv[dim] -= edge_deriv[dim];
    }

    size = GET_OBJECT_SIZE( *model, *which_triangle );
    for_less( v, 0, size )
    {
        vertex[v] = model->indices[
                       POINT_INDEX(model->end_indices,*which_triangle,v)];
    }

    get_plane_normal( size, original_points, vertex, normal );

    remove_vector_component( deriv, normal );

    mag = dot_vectors( deriv, deriv );
    if( mag == 0.0 )
        return( 0.0 );

    mag = sqrt( mag );
    deriv[X] /= mag;
    deriv[Y] /= mag;
    deriv[Z] /= mag;

    max_dist = -1.0;
    
    for_less( v, 0, size )
    {
        dist_to_edge = find_distance_to_edge( point, deriv, normal,
                                        original_points[vertex[v]],
                                        original_points[vertex[(v+1)%size]] );

        if( (dist_to_edge >= 0.0) &&
            (max_dist < 0.0 || dist_to_edge < max_dist) )
        {
            max_dist = dist_to_edge;
            exit_neighbour = v;
        }
    }

    max_dist_to_neighbour = -1.0;
    
    for_less( n, 0, n_neighbours )
    {
        dist_to_edge = find_distance_to_neighbour_edge( point, deriv,
                                        neighbours[n],
                                        neighbours[(n+1)%n_neighbours] );

        if( (dist_to_edge >= 0.0) &&
            (max_dist_to_neighbour < 0.0 ||
             dist_to_edge < max_dist_to_neighbour) )
        {
            max_dist_to_neighbour = dist_to_edge;
        }
    }

    /*----------------------------------------- */

    best_fit = evaluate_fit( point, n_neighbours, neighbours, lengths, normal );

    max_step = MIN( max_dist, max_dist_to_neighbour * 1.0e-1 );

    step = max_dist * .1;
    position = 0.0;
    while( step > max_dist * 1e-20 && position < max_step )
    {
        new_position = position + step;
        if( new_position > max_step )
            new_position = max_step;

        for_less( dim, 0, N_DIMENSIONS )
            new_point[dim] = point[dim] + new_position * deriv[dim];

/*
        len = sqrt( new_point[X] * new_point[X] +
                    new_point[Y] * new_point[Y] +
                    new_point[Z] * new_point[Z] );

        for_less( dim, 0, N_DIMENSIONS )
            new_point[dim] /= len;
*/

        new_fit = evaluate_fit( new_point, n_neighbours, neighbours, lengths,
                                normal );

        if( new_fit < best_fit )
        {
            best_fit = new_fit;
            position = new_position;
        }
        else
        {
            step *= 0.1;
        }
    }

    if( position == max_dist )
    {
        *which_triangle = model->neighbours[
           POINT_INDEX(model->end_indices,*which_triangle,exit_neighbour)];
    }
    else
        position *= ratio;


    for_less( dim, 0, N_DIMENSIONS )
        new_point[dim] = point[dim] + position * deriv[dim];

    dx = (new_point[X] - point[X]);
    dy = (new_point[Y] - point[Y]);
    dz = (new_point[Z] - point[Z]);
    movement = dx * dx + dy * dy + dz * dz;
    if( movement > 0.0 )
        movement = sqrt( movement );

    for_less( dim, 0, N_DIMENSIONS )
        point[dim] = new_point[dim];

    return( movement );
}

private  Real  perturb_vertices(
    polygons_struct   *model,
    Real              (*original_points)[N_DIMENSIONS],
    Real              model_lengths[],
    int               n_neighbours[],
    int               *neighbours[],
    Real              (*new_points)[N_DIMENSIONS],
    int               which_triangle[],
    Real              ratio )
{
    int    d, point_index, max_neighbours, n, ind;
    Real   movement, max_movement, *lengths, (*points)[N_DIMENSIONS];

    max_neighbours = 0;
    for_less( point_index, 0, model->n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[point_index] );

    ALLOC( points, max_neighbours );
    ALLOC( lengths, max_neighbours );

    max_movement = 0.0;
    ind = n_neighbours[0];
    for_less( point_index, 1, model->n_points )
    {
        for_less( n, 0, n_neighbours[point_index] )
        {
            lengths[n] = model_lengths[ind];
            ++ind;
            for_less( d, 0, N_DIMENSIONS )
                points[n][d] = new_points[neighbours[point_index][n]][d];
        }

        movement = optimize_vertex( model, original_points,
                                    new_points[point_index],
                                    n_neighbours[point_index],
                                    points, lengths,
                                    &which_triangle[point_index], ratio );

        max_movement = MAX( max_movement, movement );
    }

    FREE( points );
    FREE( lengths );

    return( max_movement );
}

private  void  get_errors(
    int   n_points,
    int   n_neighbours[],
    int   *neighbours[],
    Real  (*points)[N_DIMENSIONS],
    Real  model_lengths[],
    Real  *max_error,
    Real  *avg_error )
{
    int    ind, point, n;
    Real   dx, dy, dz, dist, diff;

    *max_error = 0.0;
    *avg_error = 0.0;

    ind = 0;
    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            dx = points[point][X] - points[neighbours[point][n]][X];
            dy = points[point][Y] - points[neighbours[point][n]][Y];
            dz = points[point][Z] - points[neighbours[point][n]][Z];
            dist = dx * dx + dy * dy + dz * dz;

            if( dist > 0.0 )  dist = sqrt( dist );

            diff = dist - model_lengths[ind];
            diff = FABS( diff ) / model_lengths[ind];
            *max_error = MAX( diff, *max_error );
            *avg_error += diff;
            ++ind;
        }
    }

    *avg_error /= (Real) ind;
}

private  void  reparameterize_by_minimization(
    polygons_struct   *model,
    int               n_neighbours[],
    int               *neighbours[],
    Real              model_lengths[],
    Real              (*original_points)[N_DIMENSIONS],
    Real              (*new_points)[N_DIMENSIONS],
    int               which_triangle[],
    Real              ratio,
    Real              movement_threshold,
    int               n_iters )
{
    int   iter;
    Real  movement;
    Real  max_error, avg_error;

    iter = 0;
    movement = -1.0;
    while( iter < n_iters && (movement < 0.0 || movement >= movement_threshold))
    {
        movement = perturb_vertices( model, original_points,
                                     model_lengths,
                                     n_neighbours, neighbours,
                                     new_points, which_triangle, ratio );

        get_errors( model->n_points, n_neighbours, neighbours,
                    new_points, model_lengths, &max_error, &avg_error );

        ++iter;
        print( "%3d: %20g\t%20g\t%20g\n", iter, movement, avg_error, max_error );
    }
}
#endif

private  void  get_normalized_uv(
    Real   u,
    Real   v,
    Real   *uu,
    Real   *vv )
{
    while( u < 0.0 )
        u += 1.0;
    while( u >= 1.0 )
        u -= 1.0;

    while( v < 0.0 )
        v += 2.0;
    while( v >= 2.0 )
        v -= 2.0;

    if( v > 1.0 )
        v = 2.0 - v;

    *uu = u;
    *vv = v;
}

private  Real  evaluate_fit(
    polygons_struct   *original,
    polygons_struct   *unit_sphere,
    Real              parameters[],
    int               n_neighbours[],
    int               *neighbours[],
    Real              model_lengths[] )
{
    int    point, n, neigh, ind, n_points;
    Real   fit, x, y, z, x1, y1, z1, x2, y2, z2, model_len, actual_len;
    Real   (*points)[N_DIMENSIONS], len, dx, dy, dz;
    Point  surface_point, unit_sphere_point;

    n_points = original->n_points;

    ALLOC( points, n_points );

    for_less( point, 0, n_points )
    {
        map_uv_to_sphere( parameters[IJ(point,0,2)],
                          parameters[IJ(point,1,2)],
                          &x, &y, &z );

        fill_Point( unit_sphere_point, x, y, z );

        map_unit_sphere_to_point( unit_sphere, &unit_sphere_point,
                                  original, &surface_point );
        points[point][X] = RPoint_x(surface_point);
        points[point][Y] = RPoint_y(surface_point);
        points[point][Z] = RPoint_z(surface_point);
    }

    fit = 0.0;
    ind = 0;

    for_less( point, 0, n_points )
    {
        x1 = points[point][X];
        y1 = points[point][Y];
        z1 = points[point][Z];

        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            if( neigh < point )
            {
                ++ind;
                continue;
            }

            x2 = points[neigh][X];
            y2 = points[neigh][Y];
            z2 = points[neigh][Z];

            dx = x2 - x1;
            dy = y2 - y1;
            dz = z2 - z1;

            actual_len = dx * dx + dy * dy + dz * dz;
            if( actual_len > 0.0 )
                actual_len = sqrt( actual_len );
            else
                actual_len = 0.0;

            model_len = model_lengths[ind];
            ++ind;
            len = (actual_len - model_len) / model_len;
            fit += len * len;
        }
    }

    FREE( points );

    return( fit );
}

typedef  struct
{
    polygons_struct   *original;
    polygons_struct   *unit_sphere;
    int               *n_neighbours;
    int               **neighbours;
    Real              *model_lengths;
} function_data;

private  Real  eval_function(
    void    *void_data,
    float   parameters[] )
{
    int             p;
    Real            *real_parameters, fit, u, v;
    function_data   *data;

    data = (function_data *) void_data;

    ALLOC( real_parameters, 2 * data->original->n_points );
    for_less( p, 0, data->original->n_points )
    {
        get_normalized_uv( (Real) parameters[IJ(p,0,2)],
                           (Real) parameters[IJ(p,1,2)], &u, &v );
        real_parameters[IJ(p,0,2)] = u;
        real_parameters[IJ(p,1,2)] = v;
    }

    fit = evaluate_fit( data->original, data->unit_sphere,
                        real_parameters, data->n_neighbours,
                        data->neighbours, data->model_lengths );

    FREE( real_parameters );

    return( fit );
}

#ifdef OLD
private  void  evaluate_fit_deriv(
    polygons_struct   *original,
    polygons_struct   *unit_sphere,
    Real              parameters[],
    int               n_neighbours[],
    int               *neighbours[],
    Real              model_lengths[],
    Real              deriv[] )
{
    int    point, n, neigh, ind, n_points;
    Real   x, y, z, x1, y1, z1, x2, y2, z2, model_len, actual_len;
    Real   (*points)[N_DIMENSIONS], len, dx, dy, dz;
    Point  surface_point, unit_sphere_point;

    n_points = original->n_points;

    for_less( point, 0, 2*n_points )
        deriv[point] = 0.0;

    ALLOC( points, n_points );

    for_less( point, 0, n_points )
    {
        map_uv_to_sphere( parameters[IJ(point,0,2)],
                          parameters[IJ(point,1,2)],
                          &x, &y, &z );

        fill_Point( unit_sphere_point, x, y, z );

        map_unit_sphere_to_point( unit_sphere, &unit_sphere_point,
                                  original, &surface_point );
        points[point][X] = RPoint_x(surface_point);
        points[point][Y] = RPoint_y(surface_point);
        points[point][Z] = RPoint_z(surface_point);
    }

    ind = 0;

    for_less( point, 0, n_points )
    {
        x1 = points[point][X];
        y1 = points[point][Y];
        z1 = points[point][Z];

        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            if( neigh < point )
            {
                ++ind;
                continue;
            }

            x2 = points[neigh][X];
            y2 = points[neigh][Y];
            z2 = points[neigh][Z];

            dx = x2 - x1;
            dy = y2 - y1;
            dz = z2 - z1;

            actual_len = dx * dx + dy * dy + dz * dz;
            if( actual_len > 0.0 )
                actual_len = sqrt( actual_len );
            else
                actual_len = 0.0;

            model_len = model_lengths[ind];
            ++ind;
            len = (actual_len - model_len) / model_len;

            factor = 2.0 * len / model_length / actual_len;

            deriv[ind0+0] += factor * -dx;
            deriv[ind0+1] += factor * -dy;
            deriv[ind0+2] += factor * -dz;
            deriv[ind1+0] += factor * dx;
            deriv[ind1+1] += factor * dy;
            deriv[ind1+2] += factor * dz;

        }
    }

    FREE( points );

    return( fit );
}
#endif

private  void  reparameterize(
    polygons_struct   *original,
    object_struct     *initial,
    polygons_struct   *model,
    Real              scale,
    Real              tolerance,
    int               n_iters )
{
    int               total_neighbours, ind, point, n, obj_index;
    int               *n_neighbours, **neighbours, dim, iter;
    Real              *model_lengths, fit, *deltas, u, v;
    Real              *parameters, x, y, z;
    Real              total_model, total_original, test_fit, rms;
    Point             *new_points, unit_sphere_point, centre;
    Point             original_point;
    object_struct     *object;
    int               update_interval;
    Real              last_update;
    polygons_struct   unit_sphere;
    amoeba_struct     amoeba;
    function_data     data;

    object = create_object( POLYGONS );
    *get_polygons_ptr(object) = *original;

    create_polygon_point_neighbours( original, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    check_polygons_neighbours_computed( model );

    total_neighbours = 0;
    for_less( point, 0, original->n_points )
        total_neighbours += n_neighbours[point];

    ALLOC( model_lengths, total_neighbours );
    ind = 0;

    total_model = 0.0;
    total_original = 0.0;

    for_less( point, 0, original->n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            model_lengths[ind] = distance_between_points( &original->points[point],
                                   &original->points[neighbours[point][n]] );
            total_original += model_lengths[ind];
            total_model += distance_between_points( &model->points[point],
                                   &model->points[neighbours[point][n]] );
            ++ind;
        }
    }

    if( scale <= 0.0 )
    {
        scale = total_model / total_original;
        print( "Scale: %g\n", scale );
    }

    for_less( ind, 0, total_neighbours )
        model_lengths[ind] *= scale;

    ALLOC( parameters, 2 * original->n_points );

    fill_Point( centre, 0.0, 0.0, 0.0 );
    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0, original->n_items,
                               &unit_sphere );

    create_polygons_bintree( &unit_sphere,
                             ROUND( (Real) unit_sphere.n_items*BINTREE_FACTOR));

    create_polygons_bintree( original,
                             ROUND( (Real) unit_sphere.n_items*BINTREE_FACTOR));

    for_less( point, 0, original->n_points )
    {
        (void) find_closest_point_on_object( &get_polygons_ptr(initial)->
                                              points[point],
                                             object, &obj_index,
                                             &original_point );

        map_point_to_unit_sphere( original, &original_point,
                                  &unit_sphere, &unit_sphere_point );
                                    
        map_sphere_to_uv( RPoint_x(unit_sphere_point),
                          RPoint_y(unit_sphere_point),
                          RPoint_z(unit_sphere_point),
                          &parameters[IJ(point,0,2)],
                          &parameters[IJ(point,1,2)] );
    }

    delete_bintree( original->bintree );
    original->bintree = NULL;

    fit = evaluate_fit( original, &unit_sphere,
                        parameters, n_neighbours, neighbours,
                        model_lengths );

    print( "Initial fit: %.6g\n", fit );

/*
    ALLOC( deltas, 2 * original->n_points );

    for_less( point, 0, 2 * original->n_points )
        deltas[point] = 0.01;

    data.original = original;
    data.unit_sphere = &unit_sphere;
    data.n_neighbours = n_neighbours;
    data.neighbours = neighbours;
    data.model_lengths = model_lengths;

    initialize_amoeba( &amoeba, 2 * original->n_points, parameters,
                       deltas, eval_function, (void *) &data,
                       tolerance );

    FREE( deltas );

    for_less( iter, 0, n_iters )
    {
        if( !perform_amoeba( &amoeba ) )
            break;
    }

    (void) get_amoeba_parameters( &amoeba, parameters );

    terminate_amoeba( &amoeba );
*/

    ALLOC( deltas, 2 * original->n_points );

    last_update = current_realtime_seconds();
    update_interval = 1;

    for_less( iter, 0, n_iters )
    {
        for_less( point, 0, 2 * original->n_points )
        {
            deltas[point] = parameters[point] + (2.0 * get_random_0_to_1() - 1.0)
                            * tolerance; 
        }

        for_less( point, 0, original->n_points )
        {
            get_normalized_uv( deltas[IJ(point,0,2)],
                               deltas[IJ(point,1,2)],
                               &deltas[IJ(point,0,2)],
                               &deltas[IJ(point,1,2)] );
        }

        test_fit = evaluate_fit( original, &unit_sphere,
                                 deltas, n_neighbours, neighbours,
                                 model_lengths );

        if( test_fit < fit )
        {
            fit = test_fit;
            for_less( point, 0, 2 * original->n_points )
                parameters[point] = deltas[point];
        }

        if( iter == n_iters-1 || ((iter+1) % update_interval) == 0 )
        {
            if( current_realtime_seconds() - last_update < 1.0 )
                update_interval *= 10;

            last_update = current_realtime_seconds();
 
            print( "%d: %.8g %.8g\n", iter+1, fit, test_fit );
        }
    }

    FREE( deltas );

    print( "After %d iterations: %.6g\n", iter, fit );

    delete_polygon_point_neighbours( original, n_neighbours,
                                     neighbours, NULL, NULL );

    FREE( model_lengths );

    ALLOC( new_points, original->n_points );

    for_less( point, 0, original->n_points )
    {
        get_normalized_uv( parameters[IJ(point,0,2)],
                           parameters[IJ(point,1,2)], &u, &v );
        map_uv_to_sphere( u, v, &x, &y, &z );
        fill_Point( unit_sphere_point, x, y, z );

        map_unit_sphere_to_point( &unit_sphere, &unit_sphere_point,
                                  original, &new_points[point] );
    }

    rms = 0;
    for_less( point, 0, original->n_points )
    {
        rms += sq_distance_between_points( &original->points[point],
                                           &new_points[point] );
        original->points[point] = new_points[point];
    }

    rms /= (Real) original->n_points;
    if( rms > 0.0 )
        rms = sqrt( rms );

    print( "RMS  %g\n", rms );

    delete_polygons( &unit_sphere );
    FREE( parameters );
    FREE( new_points );
}
