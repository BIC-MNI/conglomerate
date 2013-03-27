#include  <volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR 0.4

private  void  reparameterize(
    polygons_struct   *original,
    polygons_struct   *model,
    int               method,
    int               grid_size,
    VIO_Real              ratio,
    VIO_Real              movement_threshold,
    int               n_iters );

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               input_filename, output_filename;
    VIO_STR               model_filename;
    File_formats         src_format, model_format;
    int                  n_src_objects, n_model_objects;
    object_struct        **src_objects, **model_objects;
    polygons_struct      *original, *model;
    VIO_Real                 movement_threshold, ratio;
    int                  n_iters, method, grid_size;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &model_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  input.obj  model.obj output.obj\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1, &n_iters );
    (void) get_real_argument( 1.0, &ratio );
    (void) get_int_argument( 0, &method );
    (void) get_int_argument( 30, &grid_size );
    (void) get_real_argument( 0.0, &movement_threshold );

    if( input_graphics_file( input_filename, &src_format,
                             &n_src_objects, &src_objects ) != VIO_OK ||
        n_src_objects != 1 || get_object_type(src_objects[0]) != POLYGONS )
    {
        print_error( "Error in %s\n", input_filename );
        return( 1 );
    }

    if( input_graphics_file( model_filename, &model_format,
                             &n_model_objects, &model_objects ) != VIO_OK ||
        n_model_objects != 1 || get_object_type(model_objects[0]) != POLYGONS )
    {
        print_error( "Error in %s\n", model_filename );
        return( 1 );
    }

    original = get_polygons_ptr( src_objects[0] );
    model = get_polygons_ptr( model_objects[0] );

    if( !objects_are_same_topology( original->n_points,
                                    original->n_items,
                                    original->end_indices,
                                    original->indices,
                                    model->n_points,
                                    model->n_items,
                                    model->end_indices,
                                    model->indices ) )
    {
        print_error( "Mismatched topology.\n" );
        return( 1 );
    }

    reparameterize( original, model, method, grid_size, ratio,
                    movement_threshold, n_iters );

    if( output_graphics_file( output_filename, src_format, n_src_objects,
                              src_objects ) != VIO_OK )
        return( 1 );

    delete_object_list( n_src_objects, src_objects );

    output_alloc_to_file( NULL );

    return( 0 );
}

private  VIO_Real  dot_vectors(
    VIO_Real  v1[],
    VIO_Real  v2[] )
{
    return( v1[X] * v2[X] + v1[Y] * v2[Y] + v1[Z] * v2[Z] );
}

private  void  sub_vectors(
    VIO_Real  v1[],
    VIO_Real  v2[],
    VIO_Real  sub[] )
{
    sub[X] = v1[X] - v2[X];
    sub[Y] = v1[Y] - v2[Y];
    sub[Z] = v1[Z] - v2[Z];
}

private  void  cross_vectors(
    VIO_Real  v1[],
    VIO_Real  v2[],
    VIO_Real  c[] )
{
    c[X] = v1[Y] * v2[Z] - v1[Z] * v2[Y];
    c[Y] = v1[Z] * v2[X] - v1[X] * v2[Z];
    c[Z] = v1[X] * v2[Y] - v1[Y] * v2[X];
}

private  VIO_Real  evaluate_fit(
    VIO_Real    point[],
    int     n_neighbours,
    VIO_Real    (*neighbours)[N_DIMENSIONS],
    VIO_Real    lengths[],
    VIO_Real    normal[] )
{
    int    n, next_n;
    VIO_Real   v1[N_DIMENSIONS], v2[N_DIMENSIONS];
    VIO_Real   test_normal[N_DIMENSIONS];
    VIO_Real   fit, dist, diff, dx, dy, dz, len_t, len_n;

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
    VIO_Real    point[],
    VIO_Real    length,
    VIO_Real    neighbour[],
    VIO_Real    deriv[] )
{
    VIO_Real   dist, diff, dx, dy, dz, factor;

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
    VIO_Real    (*points)[N_DIMENSIONS],
    int     indices[],
    VIO_Real    normal[] )
{
    int     i, next_i;
    VIO_Real    vx, vy, vz, x, y, z, tx, ty, tz;

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
    VIO_Real    direction[],
    VIO_Real    plane_normal[] )
{
    VIO_Real   factor, bottom;

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

private  VIO_Real  find_distance_to_edge(
    VIO_Real    origin[],
    VIO_Real    direction[],
    VIO_Real    plane_normal[],
    VIO_Real    p1[],
    VIO_Real    p2[] )
{
    VIO_Real   dist;
    VIO_Real   edge[N_DIMENSIONS];
    VIO_Real   ortho_dir[N_DIMENSIONS];
    VIO_Real   test_normal[N_DIMENSIONS];

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

private  VIO_Real  find_distance_to_neighbour_edge(
    VIO_Real    origin[],
    VIO_Real    direction[],
    VIO_Real    p1[],
    VIO_Real    p2[] )
{
    VIO_Real   dist;
    VIO_Real   edge[N_DIMENSIONS];
    VIO_Real   ortho_dir[N_DIMENSIONS];

    sub_vectors( p2, p1, edge );

    ortho_dir[X] = direction[X];
    ortho_dir[Y] = direction[Y];
    ortho_dir[Z] = direction[Z];
    remove_vector_component( ortho_dir, edge );

    dist = dot_vectors( ortho_dir, p1 ) - dot_vectors( ortho_dir, origin );

    return( dist );
}

private  VIO_Real  optimize_vertex(
    polygons_struct   *model,
    VIO_Real              (*original_points)[N_DIMENSIONS],
    VIO_Real              point[],
    int               n_neighbours,
    VIO_Real              (*neighbours)[N_DIMENSIONS],
    VIO_Real              lengths[],
    int               *which_triangle,
    VIO_Real              ratio )
{
    int    n, dim, v, vertex[10000], size, exit_neighbour;
    VIO_Real   deriv[N_DIMENSIONS], edge_deriv[N_DIMENSIONS], movement, mag;
    VIO_Real   normal[N_DIMENSIONS], max_dist, dist_to_edge;
    VIO_Real   new_point[N_DIMENSIONS];
    VIO_Real   dx, dy, dz, step, len, max_dist_to_neighbour, max_step;
    VIO_Real   best_fit, new_fit, position, new_position;

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

private  VIO_Real  perturb_vertices(
    polygons_struct   *model,
    VIO_Real              (*original_points)[N_DIMENSIONS],
    VIO_Real              model_lengths[],
    int               n_neighbours[],
    int               *neighbours[],
    VIO_Real              (*new_points)[N_DIMENSIONS],
    int               which_triangle[],
    VIO_Real              ratio )
{
    int    d, point_index, max_neighbours, n, ind;
    VIO_Real   movement, max_movement, *lengths, (*points)[N_DIMENSIONS];

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
    VIO_Real  (*points)[N_DIMENSIONS],
    VIO_Real  model_lengths[],
    VIO_Real  *max_error,
    VIO_Real  *avg_error )
{
    int    ind, point, n;
    VIO_Real   dx, dy, dz, dist, diff;

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

    *avg_error /= (VIO_Real) ind;
}

private  void  reparameterize_by_minimization(
    polygons_struct   *model,
    int               n_neighbours[],
    int               *neighbours[],
    VIO_Real              model_lengths[],
    VIO_Real              (*original_points)[N_DIMENSIONS],
    VIO_Real              (*new_points)[N_DIMENSIONS],
    int               which_triangle[],
    VIO_Real              ratio,
    VIO_Real              movement_threshold,
    int               n_iters )
{
    int   iter;
    VIO_Real  movement;
    VIO_Real  max_error, avg_error;

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

private  void  reparameterize(
    polygons_struct   *original,
    polygons_struct   *model,
    int               method,
    int               grid_size,
    VIO_Real              ratio,
    VIO_Real              movement_threshold,
    int               n_iters )
{
    int   total_neighbours, ind, point, n, *n_neighbours, **neighbours, dim;
    int   *which_triangle, size, vertex, poly;
    VIO_Real  *model_lengths, (*new_points)[N_DIMENSIONS];
    VIO_Real  (*original_points)[N_DIMENSIONS];
    VIO_Real  scale, total_model, total_original;
    VIO_Real  max_error, avg_error;
    VIO_Point             *model_points;

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

    scale = total_model / total_original;
    for_less( ind, 0, total_neighbours )
        model_lengths[ind] *= scale;

    ALLOC( original_points, original->n_points );
    ALLOC( new_points, original->n_points );

    for_less( point, 0, original->n_points )
    for_less( dim, 0, N_DIMENSIONS )
    {
        original_points[point][dim] =
                         RPoint_coord(model->points[point],dim);
        new_points[point][dim] = original_points[point][dim];
    }


    ALLOC( which_triangle, original->n_points );
    for_less( point, 0, original->n_points )
        which_triangle[point] = -1;

    for_less( poly, 0, original->n_items )
    {
        size = GET_OBJECT_SIZE( *original, poly );
        for_less( vertex, 0, size )
        {
            point = original->indices[
                   POINT_INDEX(original->end_indices,poly,vertex)];
            if( which_triangle[point] < 0 )
                which_triangle[point] = poly;
        }
    }

    get_errors( original->n_points, n_neighbours, neighbours,
                new_points, model_lengths, &max_error, &avg_error );

    print( "Initial Avg error: %g\t", avg_error );
    print( "Initial Max error: %g\n\n", max_error );

    reparameterize_by_minimization( model, n_neighbours,
                                    neighbours, model_lengths,
                                    original_points, new_points,
                                    which_triangle,
                                    ratio, movement_threshold, n_iters );

    get_errors( original->n_points, n_neighbours, neighbours,
                new_points, model_lengths, &max_error, &avg_error );

    print( "Avg error: %g\t", avg_error );
    print( "Max error: %g\n", max_error );

    delete_polygon_point_neighbours( original, n_neighbours,
                                     neighbours, NULL, NULL );

    FREE( model_lengths );
    FREE( original_points );
    FREE( which_triangle );

    ALLOC( model_points, original->n_points );
    for_less( point, 0, original->n_points )
    {
        model_points[point] = model->points[point];
        fill_Point( model->points[point],
                    new_points[point][X], new_points[point][Y],
                    new_points[point][Z] );
    }

    create_polygons_bintree( model,
                         ROUND( BINTREE_FACTOR * (VIO_Real) original->n_items ) );

    for_less( point, 0, original->n_points )
    {
        map_unit_sphere_to_point( model, &model_points[point],
                                  original, &original->points[point] );
    }

    FREE( new_points );
    FREE( model_points );
}
