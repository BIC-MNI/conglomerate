#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR 0.4

private  void  reparameterize(
    polygons_struct   *original,
    int               method,
    int               grid_size,
    Real              ratio,
    Real              movement_threshold,
    int               n_iters );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename;
    File_formats         src_format;
    int                  n_src_objects;
    object_struct        **src_objects;
    polygons_struct      *original;
    Real                 movement_threshold, ratio;
    int                  n_iters, method, grid_size;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  input.obj  output.obj\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1, &n_iters );
    (void) get_real_argument( 1.0, &ratio );
    (void) get_int_argument( 0, &method );
    (void) get_int_argument( 30, &grid_size );
    (void) get_real_argument( 0.0, &movement_threshold );

    if( input_graphics_file( input_filename, &src_format,
                             &n_src_objects, &src_objects ) != OK ||
        n_src_objects != 1 || get_object_type(src_objects[0]) != POLYGONS )
    {
        print_error( "Error in %s\n", input_filename );
        return( 1 );
    }

    original = get_polygons_ptr( src_objects[0] );

    reparameterize( original, method, grid_size, ratio,
                    movement_threshold, n_iters );

    if( output_graphics_file( output_filename, src_format, n_src_objects,
                              src_objects ) != OK )
        return( 1 );

    delete_object_list( n_src_objects, src_objects );

    output_alloc_to_file( NULL );

    return( 0 );
}

private  void  map_2d_to_3d(
    polygons_struct   *unit_sphere,
    polygons_struct   *original,
    Real              u,
    Real              v,
    Real              *x,
    Real              *y,
    Real              *z )
{
    Real    x_sphere, y_sphere, z_sphere;
    Point   unit_sphere_point, original_point;

    while( u < 0.0 )
        u += 1.0;
    while( u >= 1.0 )
        u -= 1.0;

    if( v > 1.0 )
        v = 2.0 - v;
    if( v < 0.0 )
        v = -v;

    map_uv_to_sphere( u, v, &x_sphere, &y_sphere, &z_sphere );

    fill_Point( unit_sphere_point, x_sphere, y_sphere, z_sphere );

    map_unit_sphere_to_point( unit_sphere, &unit_sphere_point,
                              original, &original_point );

    *x = RPoint_x( original_point );
    *y = RPoint_y( original_point );
    *z = RPoint_z( original_point );
}

private  void  map_3d_to_2d(
    polygons_struct   *unit_sphere,
    polygons_struct   *original,
    Real              x,
    Real              y,
    Real              z,
    Real              *u,
    Real              *v )
{
    Point   unit_sphere_point, original_point;

    fill_Point( original_point, x, y, z );
    map_point_to_unit_sphere( original, &original_point,
                              unit_sphere, &unit_sphere_point );

    map_sphere_to_uv( RPoint_x(unit_sphere_point),
                      RPoint_y(unit_sphere_point),
                      RPoint_z(unit_sphere_point),
                      u, v );
}

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
    Real    lengths[] )
{
    int    n, next_n;
    Real   v1[N_DIMENSIONS], v2[N_DIMENSIONS], normal[N_DIMENSIONS];
    Real   fit, dist, diff, dx, dy, dz, len_p, len_n;

    fit = 0.0;

    len_p = dot_vectors( point, point );

    for_less( n, 0, n_neighbours )
    {
        next_n = (n + 1) % n_neighbours;

        sub_vectors( neighbours[n], point, v1 );
        sub_vectors( neighbours[next_n], point, v2 );
        cross_vectors( v1, v2, normal );
        len_n = dot_vectors( normal, normal );
        if( dot_vectors( point, normal ) / sqrt( len_n * len_p ) <= 0.1 )
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
    polygons_struct   *unit_sphere,
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

    size = GET_OBJECT_SIZE( *unit_sphere, *which_triangle );
    for_less( v, 0, size )
    {
        vertex[v] = unit_sphere->indices[
                       POINT_INDEX(unit_sphere->end_indices,*which_triangle,v)];
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

    best_fit = evaluate_fit( point, n_neighbours, neighbours, lengths );

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

        new_fit = evaluate_fit( new_point, n_neighbours, neighbours, lengths );

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
        *which_triangle = unit_sphere->neighbours[
           POINT_INDEX(unit_sphere->end_indices,*which_triangle,exit_neighbour)];
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
    polygons_struct   *unit_sphere,
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
    for_less( point_index, 0, unit_sphere->n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[point_index] );

    ALLOC( points, max_neighbours );
    ALLOC( lengths, max_neighbours );

    max_movement = 0.0;
    ind = n_neighbours[0];
    for_less( point_index, 1, unit_sphere->n_points )
    {
        for_less( n, 0, n_neighbours[point_index] )
        {
            lengths[n] = model_lengths[ind];
            ++ind;
            for_less( d, 0, N_DIMENSIONS )
                points[n][d] = new_points[neighbours[point_index][n]][d];
        }

        movement = optimize_vertex( unit_sphere, original_points,
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
    polygons_struct   *unit_sphere,
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
        movement = perturb_vertices( unit_sphere, original_points,
                                     model_lengths,
                                     n_neighbours, neighbours,
                                     new_points, which_triangle, ratio );

        get_errors( unit_sphere->n_points, n_neighbours, neighbours,
                    new_points, model_lengths, &max_error, &avg_error );

        ++iter;
        print( "%3d: %20g\t%20g\t%20g\n", iter, movement, avg_error, max_error );
    }
}

private  void  reparameterize(
    polygons_struct   *original,
    int               method,
    int               grid_size,
    Real              ratio,
    Real              movement_threshold,
    int               n_iters )
{
    int   total_neighbours, ind, point, n, *n_neighbours, **neighbours, dim;
    int   *which_triangle, size, vertex, poly;
    Real  *model_lengths, (*new_points)[N_DIMENSIONS];
    Real  (*original_points)[N_DIMENSIONS];
    Real  scale, total_model, total_original;
    Real  max_error, avg_error;
    Point             centre, *unit_sphere_points;
    polygons_struct   unit_sphere;

    fill_Point( centre, 0.0, 0.0, 0.0 );
    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0, original->n_items,
                               &unit_sphere );

    create_polygon_point_neighbours( original, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    check_polygons_neighbours_computed( &unit_sphere );

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
            total_model += distance_between_points(
                                   &unit_sphere.points[point],
                                   &unit_sphere.points[neighbours[point][n]] );
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
                         RPoint_coord(unit_sphere.points[point],dim);
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

    reparameterize_by_minimization( &unit_sphere, n_neighbours,
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

    ALLOC( unit_sphere_points, original->n_points );
    for_less( point, 0, original->n_points )
    {
        unit_sphere_points[point] = unit_sphere.points[point];
        fill_Point( unit_sphere.points[point],
                    new_points[point][X], new_points[point][Y],
                    new_points[point][Z] );
    }

    create_polygons_bintree( &unit_sphere,
                         ROUND( BINTREE_FACTOR * (Real) original->n_items ) );

    for_less( point, 0, original->n_points )
    {
        map_unit_sphere_to_point( &unit_sphere, &unit_sphere_points[point],
                                  original, &original->points[point] );
    }

    delete_polygons( &unit_sphere );
    FREE( new_points );
    FREE( unit_sphere_points );
}
