#include  <volume_io.h>
#include  <bicpl.h>

#undef  USE_CENTROID
#define USE_CENTROID

typedef  float  dtype;

private  void  flatten_polygons(
    int              n_points,
    Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    Point            init_points[],
    Real             centroid_weight,
    Real             sphere_weight,
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename, initial_filename;
    int                  n_objects, n_i_objects, n_iters;
    int                  *n_neighbours, **neighbours, n_points;
    File_formats         format;
    object_struct        **object_list, **i_object_list;
    polygons_struct      *polygons, *init_polygons, p;
    Point                *init_points, *points;
    Real                 sphere_weight, centroid_weight;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj [cw] [sw] [n_iters]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 1.0, &centroid_weight );
    (void) get_real_argument( 1.0, &sphere_weight );
    (void) get_int_argument( 100, &n_iters );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    if( get_string_argument( NULL, &initial_filename ) )
    {
        if( input_graphics_file( initial_filename, &format, &n_i_objects,
                                 &i_object_list ) != OK || n_i_objects != 1 ||
            get_object_type(i_object_list[0]) != POLYGONS )
            return( 1 );

        init_polygons = get_polygons_ptr( i_object_list[0] );
        init_points = init_polygons->points;
        ALLOC( init_polygons->points, 1 );
        delete_object_list( n_i_objects, i_object_list );
    }
    else
    {
        init_points = NULL;
    }

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    points = polygons->points;
    n_points = polygons->n_points;
    ALLOC( polygons->points, 1 );
    delete_object_list( 1, object_list );

    flatten_polygons( n_points, points, n_neighbours, neighbours,
                      init_points, centroid_weight,
                      sphere_weight, n_iters );

    p.n_points = n_points;
    delete_polygon_point_neighbours( &p, n_neighbours, neighbours, NULL, NULL );

    if( input_graphics_file( src_filename, &format, &n_objects,
                         &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );
    FREE( polygons->points );
    polygons->points = points;

    if( output_graphics_file( dest_filename, format, 1, object_list ) != OK )
        print_error( "Error outputting: %s\n", dest_filename );

    return( 0 );
}

private  Real  evaluate_fit(
    int     n_parameters,
    dtype   parameters[],
    dtype   distances[],
    int     n_neighbours[],
    int     *neighbours[],
    Real    centroid_weight,
    dtype   centroid_weights[],
    Real    sphere_weight )
{
    int    p, n_points, ind, n, neigh;
    Real   fit, dx, dy, dz, dist, diff, act_dist, radius;
    Real   xc, yc, zc, x, y, z, weight;

    fit = 0.0;
    n_points = n_parameters / 3;

    ind = 0;
    for_less( p, 0, n_points )
    {
        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            if( neigh < p )
                continue;

            dist = (Real) distances[ind];
            ++ind;

            dx = (Real)parameters[IJ(p,0,3)] - (Real)parameters[IJ(neigh,0,3)];
            dy = (Real)parameters[IJ(p,1,3)] - (Real)parameters[IJ(neigh,1,3)];
            dz = (Real)parameters[IJ(p,2,3)] - (Real)parameters[IJ(neigh,2,3)];
            act_dist = dx * dx + dy * dy + dz * dz;
            diff = dist - act_dist;
            fit += diff * diff;
        }
    }

    if( centroid_weight > 0.0 )
    {
        ind = 0;
        for_less( p, 0, n_points )
        {
            xc = 0.0;
            yc = 0.0;
            zc = 0.0;
            for_less( n, 0, n_neighbours[p] )
            {
                neigh = neighbours[p][n];
                weight = (Real) centroid_weights[ind+n];
                xc += (Real) parameters[IJ(neigh,0,3)] * weight;
                yc += (Real) parameters[IJ(neigh,1,3)] * weight;
                zc += (Real) parameters[IJ(neigh,2,3)] * weight;
            }

            x = (Real) parameters[IJ(p,0,3)];
            y = (Real) parameters[IJ(p,1,3)];
            z = (Real) parameters[IJ(p,2,3)];

            dx = x - xc;
            dy = y - yc;
            dz = z - zc;

            diff = dx * dx + dy * dy + dz * dz;

            fit += centroid_weight * diff;
            ind += n_neighbours[p];
        }
    }

    if( sphere_weight > 0.0 )
    {
        radius = (Real) parameters[n_parameters-1];

        for_less( p, 0, n_points )
        {
            dx = (Real) parameters[IJ(p,0,3)];
            dy = (Real) parameters[IJ(p,1,3)];
            dz = (Real) parameters[IJ(p,2,3)];
            act_dist = dx * dx + dy * dy + dz * dz;
            diff = radius * radius - act_dist;
            fit += sphere_weight * diff * diff;
        }
    }

    return( fit );
}

private  void  evaluate_fit_derivative(
    int      n_parameters,
    dtype    parameters[],
    dtype    distances[],
    int      n_neighbours[],
    int      *neighbours[],
    Real     centroid_weight,
    dtype    centroid_weights[],
    Real     sphere_weight,
    dtype    deriv[] )
{
    int    p, n_points, ind, n, neigh, p_index, n_index;
    Real   dx, dy, dz, dist, diff, act_dist, radius, factor;
    Real   x1, y1, z1, x2, y2, z2, weight, cw4;
    Real   xc, yc, zc, x_deriv, y_deriv, z_deriv;
    Real   x_factor, y_factor, z_factor, x_diff, y_diff, z_diff;
#ifdef USE_CENTROID
    dtype  fx_factor, fy_factor, fz_factor;
#endif

    for_less( p, 0, n_parameters )
        deriv[p] = (dtype) 0.0;

    n_points = n_parameters / 3;
    if( sphere_weight > 0.0 )
        radius = (Real) parameters[n_parameters-1];

    ind = 0;
    for_less( p, 0, n_points )
    {
        p_index = IJ(p,0,3);
        x1 = (Real) parameters[p_index+0];
        y1 = (Real) parameters[p_index+1];
        z1 = (Real) parameters[p_index+2];

        x_deriv = 0.0;
        y_deriv = 0.0;
        z_deriv = 0.0;

        if( sphere_weight > 0.0 )
        {
            act_dist = x1 * x1 + y1 * y1 + z1 * z1;
            diff = act_dist - radius * radius;
            weight = 4.0 * sphere_weight * diff;
            x_deriv += weight * x1;
            y_deriv += weight * y1;
            z_deriv += weight * z1;
            deriv[n_parameters-1] += (dtype) (-weight * radius);
        }

        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            if( neigh < p )
                continue;

            dist = (Real) distances[ind];
            ++ind;

            n_index = IJ(neigh,0,3);
            x2 = (Real) parameters[n_index+0];
            y2 = (Real) parameters[n_index+1];
            z2 = (Real) parameters[n_index+2];

            dx = x1 - x2;
            dy = y1 - y2;
            dz = z1 - z2;
            act_dist = dx * dx + dy * dy + dz * dz;
            factor = 4.0 * (act_dist - dist);
            dx *= factor;
            dy *= factor;
            dz *= factor;
            x_deriv += dx;
            y_deriv += dy;
            z_deriv += dz;
            deriv[n_index+0] += (dtype) -dx;
            deriv[n_index+1] += (dtype) -dy;
            deriv[n_index+2] += (dtype) -dz;
        }

        deriv[p_index+0] += (dtype) x_deriv;
        deriv[p_index+1] += (dtype) y_deriv;
        deriv[p_index+2] += (dtype) z_deriv;
    }

    if( centroid_weight > 0.0 )
    {
        cw4 = 2.0 * centroid_weight;
        ind = 0;
        for_less( p, 0, n_points )
        {
            p_index = IJ(p,0,3);
            x1 = (Real) parameters[p_index+0];
            y1 = (Real) parameters[p_index+1];
            z1 = (Real) parameters[p_index+2];
            xc = 0.0;
            yc = 0.0;
            zc = 0.0;
            for_less( n, 0, n_neighbours[p] )
            {
                neigh = neighbours[p][n];
                n_index = IJ(neigh,0,3);
#ifdef USE_CENTROID
                xc += (Real) parameters[n_index+0];
                yc += (Real) parameters[n_index+1];
                zc += (Real) parameters[n_index+2];
#else
                weight = (Real) centroid_weights[ind+n];
                xc += (Real) parameters[n_index+0] * weight;
                yc += (Real) parameters[n_index+1] * weight;
                zc += (Real) parameters[n_index+2] * weight;
#endif
            }

#ifdef USE_CENTROID
            weight = 1.0 / (Real) n_neighbours[p];
            xc *= weight;
            yc *= weight;
            zc *= weight;
#endif

            x_diff = xc - x1;
            y_diff = yc - y1;
            z_diff = zc - z1;
            x_factor = cw4 * x_diff;
            y_factor = cw4 * y_diff;
            z_factor = cw4 * z_diff;

            deriv[p_index+0] += (dtype) (-x_factor);
            deriv[p_index+1] += (dtype) (-y_factor);
            deriv[p_index+2] += (dtype) (-z_factor);

#ifdef USE_CENTROID
            fx_factor = (dtype) (x_factor * weight);
            fy_factor = (dtype) (y_factor * weight);
            fz_factor = (dtype) (z_factor * weight);
#endif

            for_less( n, 0, n_neighbours[p] )
            {
                neigh = neighbours[p][n];
                n_index = IJ(neigh,0,3);
#ifdef USE_CENTROID
                deriv[n_index+0] += fx_factor;
                deriv[n_index+1] += fy_factor;
                deriv[n_index+2] += fz_factor;
#else
                weight = (Real) centroid_weights[ind+n];
                deriv[n_index+0] += (dtype) (x_factor * weight);
                deriv[n_index+1] += (dtype) (y_factor * weight);
                deriv[n_index+2] += (dtype) (z_factor * weight);
#endif
            }

            ind += n_neighbours[p];
        }
    }
}

private  void  evaluate_fit_along_line(
    int     n_parameters,
    dtype   parameters[],
    dtype   delta[],
    dtype   distances[],
    int     n_neighbours[],
    int     *neighbours[],
    Real    centroid_weight,
    dtype   centroid_weights[],
    Real    sphere_weight,
    Real    coefs[] )
{
    int    p, n_points, ind, n, neigh, p_index, n_index;
    Real   dx, dy, dz, dr, dist, radius, radius2;
    Real   dx1, dy1, dz1, x1, y1, z1;
    Real   x, y, z;
    Real   ax, ay, az, bx, by, bz, weight;
    Real   l0, l1, l2;
    Real   d00, d01, d02, d11, d12, d22;
    Real   s00, s01, s02, s11, s12, s22;

    d00 = 0.0;
    d01 = 0.0;
    d02 = 0.0;
    d11 = 0.0;
    d12 = 0.0;
    d22 = 0.0;

    s00 = 0.0;
    s01 = 0.0;
    s02 = 0.0;
    s11 = 0.0;
    s12 = 0.0;
    s22 = 0.0;

    n_points = n_parameters / 3;

    if( sphere_weight > 0.0 )
    {
        radius = (Real) parameters[n_parameters-1];
        radius2 = radius * radius;
        dr = (Real) delta[n_parameters-1];
    }

    ind = 0;
    for_less( p, 0, n_points )
    {
        p_index = IJ(p,0,3);
        x1 = (Real) parameters[p_index+0];
        y1 = (Real) parameters[p_index+1];
        z1 = (Real) parameters[p_index+2];
        dx1 = (Real) delta[p_index+0];
        dy1 = (Real) delta[p_index+1];
        dz1 = (Real) delta[p_index+2];

        l0 = x1 * x1 + y1 * y1 + z1 * z1 - radius2;
        l1 = x1 * dx1 + y1 * dy1 + z1 * dz1 - radius * dr;
        l2 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1 - dr * dr;

        s00 += l0 * l0;
        s01 += l0 * l1;
        s02 += l0 * l2;
        s11 += l1 * l1;
        s12 += l1 * l2;
        s22 += l2 * l2;

        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            if( neigh < p )
                continue;

            dist = (Real) distances[ind];
            ++ind;

            n_index = IJ(neigh,0,3);
            x = x1 - (Real) parameters[n_index+0];
            y = y1 - (Real) parameters[n_index+1];
            z = z1 - (Real) parameters[n_index+2];
            dx = dx1 - (Real) delta[n_index+0];
            dy = dy1 - (Real) delta[n_index+1];
            dz = dz1 - (Real) delta[n_index+2];

            l0 = x * x + y * y + z * z - dist;
            l1 = x * dx + y * dy + z * dz;
            l2 = dx * dx + dy * dy + dz * dz;

            d00 += l0 * l0;
            d01 += l0 * l1;
            d02 += l0 * l2;
            d11 += l1 * l1;
            d12 += l1 * l2;
            d22 += l2 * l2;
        }
    }

    coefs[0] = d00 + s00 * sphere_weight;
    coefs[1] = 4.0 * (d01 + s01 * sphere_weight);
    coefs[2] = 2.0 * (d02 + sphere_weight * s02) +
               4.0 * (d11 + sphere_weight * s11);
    coefs[3] = 4.0 * (d12 + sphere_weight * s12);
    coefs[4] = d22 + sphere_weight * s22;

    if( centroid_weight > 0.0 )
    {
        s00 = 0.0;
        s01 = 0.0;
        s02 = 0.0;
        s11 = 0.0;
        s12 = 0.0;
        s22 = 0.0;

        ind = 0;
        for_less( p, 0, n_points )
        {
            ax = 0.0;
            bx = 0.0;
            ay = 0.0;
            by = 0.0;
            az = 0.0;
            bz = 0.0;

            for_less( n, 0, n_neighbours[p] )
            {
                neigh = neighbours[p][n];
                n_index = IJ(neigh,0,3);
#ifdef USE_CENTROID
                ax += (Real)      delta[n_index+0];
                bx += (Real) parameters[n_index+0];
                ay += (Real)      delta[n_index+1];
                by += (Real) parameters[n_index+1];
                az += (Real)      delta[n_index+2];
                bz += (Real) parameters[n_index+2];
#else
                weight = (Real) centroid_weights[ind+n];
                ax += (Real)      delta[n_index+0] * weight;
                bx += (Real) parameters[n_index+0] * weight;
                ay += (Real)      delta[n_index+1] * weight;
                by += (Real) parameters[n_index+1] * weight;
                az += (Real)      delta[n_index+2] * weight;
                bz += (Real) parameters[n_index+2] * weight;
#endif
            }

#ifdef USE_CENTROID
            weight = 1.0 / (Real) n_neighbours[p];
            ax *= weight;
            bx *= weight;
            ay *= weight;
            by *= weight;
            az *= weight;
            bz *= weight;
#endif

            p_index = IJ( p, 0, 3 );
            ax += (Real)      -delta[p_index+0];
            bx += (Real) -parameters[p_index+0];
            ay += (Real)      -delta[p_index+1];
            by += (Real) -parameters[p_index+1];
            az += (Real)      -delta[p_index+2];
            bz += (Real) -parameters[p_index+2];

            s00 += bx * bx + by * by + bz * bz;
            s01 += bx * ax + by * ay + bz * az;
            s11 += ax * ax + ay * ay + az * az;

            ind += n_neighbours[p];
        }

        coefs[0] += s00 * centroid_weight;
        coefs[1] += 2.0 * s01 * centroid_weight;
        coefs[2] += s11 * centroid_weight;
    }
}

private  void  minimize_along_line(
    int     n_parameters,
    dtype   parameters[],
    dtype   delta[],
    dtype   distances[],
    int     n_neighbours[],
    int     *neighbours[],
    Real    centroid_weight,
    dtype   centroid_weights[],
    Real    sphere_weight )
{
    int    p, s, n_solutions, best_index;
    Real   coefs[5], deriv[4], *test, t, fit, best_fit, solutions[3];
    Real   orig_fit;

    ALLOC( test, n_parameters );

    evaluate_fit_along_line( n_parameters, parameters, delta, distances,
                             n_neighbours, neighbours, centroid_weight,
                             centroid_weights, sphere_weight, coefs );

    for_less( p, 0, 4 )
        deriv[p] = (Real) (p+1) * coefs[p+1];

    n_solutions = solve_cubic( deriv[3], deriv[2], deriv[1], deriv[0],
                               solutions );

    if( n_solutions == 0 )
        print( "N solutions = 0\n" );

    orig_fit = evaluate_fit( n_parameters, parameters, distances,
                             n_neighbours, neighbours, centroid_weight,
                             centroid_weights, sphere_weight );
    best_fit = orig_fit;
    best_index = -1;

    for_less( s, 0, n_solutions )
    {
        t = solutions[s];

        fit = coefs[0] + t * (coefs[1] + t * (coefs[2] + t * (coefs[3] +
              t * coefs[4])));

        if( fit < best_fit )
        {
            best_fit = fit;
            best_index = s;
        }
    }

    if( best_index >= 0 )
    {
        t = solutions[best_index];

        for_less( p, 0, n_parameters )
            parameters[p] = (dtype) ((Real) parameters[p] + t*(Real) delta[p]);

        fit = evaluate_fit( n_parameters, parameters, distances,
                            n_neighbours, neighbours, centroid_weight,
                            centroid_weights, sphere_weight );

        if( fit > orig_fit )
        {
/*
            print( "Correcting %g > %g becomes %g\n", fit, best_fit, orig_fit );
*/
            for_less( p, 0, n_parameters )
                parameters[p] = (dtype) ((Real) parameters[p] - t*(Real) delta[p]);
        }
    }

    if( sphere_weight > 0.0 && parameters[n_parameters-1] < (dtype) 0.0 )
        parameters[n_parameters-1] *= (dtype) -1.0;

    FREE( test );
}

private  void  flatten_polygons(
    int              n_points,
    Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    Point            init_points[],
    Real             centroid_weight,
    Real             sphere_weight,
    int              n_iters )
{
    int              p, n, point, dim1, dim2, max_neighbours;
    int              n_parameters, total_neighbours, total_edges;
    Real             gg, dgg, gam, current_time, last_update_time;
    Real             fit;
    Real             len, radius, *weights[3][3], *x_flat, *y_flat, *z_flat;
    Point            centroid, *plane_points;
#ifndef   USE_CENTROID
    int              neigh;
    Vector           offset, normal, hor, vert;
#endif
    int              iter, ind, update_rate, shake_every, n_same_size;
    Real             shake_ratio;
    dtype            *g, *h, *xi, *parameters, *unit_dir, *distances;
    dtype            *centroid_weights;
    BOOLEAN          init_supplied;

    init_supplied = (init_points != NULL);
    if( !init_supplied )
        init_points = points;

    if( sphere_weight > 0.0 )
    {
        radius = 0.0;
        get_points_centroid( n_points, init_points, &centroid );
        for_less( point, 0, n_points )
        {
            radius += distance_between_points( &points[point], &centroid );
        }
        radius /= (Real) n_points;
    }

    total_neighbours = 0;
    total_edges = 0;
    for_less( point, 0, n_points )
    {
        total_neighbours += n_neighbours[point];
        for_less( n, 0, n_neighbours[point] )
            if( neighbours[point][n] > point )
                ++total_edges;
    }

    if( centroid_weight > 0.0 )
    {
        max_neighbours = 0;
        for_less( point, 0, n_points )
            max_neighbours = MAX( max_neighbours, n_neighbours[point] );

        for_less( dim1, 0, 3 )
        for_less( dim2, 0, 3 )
            ALLOC( weights[dim1][dim2], max_neighbours );
        ALLOC( x_flat, max_neighbours );
        ALLOC( y_flat, max_neighbours );
        ALLOC( z_flat, max_neighbours );
        ALLOC( plane_points, max_neighbours );

        ALLOC( centroid_weights, total_neighbours );
        ind = 0;
        for_less( point, 0, n_points )
        {
#ifdef USE_CENTROID
            for_less( n, 0, n_neighbours[point] )
                centroid_weights[ind+n] = (dtype)
                            (1.0 / (Real) n_neighbours[point]);
#else
            for_less( n, 0, n_neighbours[point] )
                plane_points[n] = points[neighbours[point][n]];

            find_polygon_normal( n_neighbours[point], plane_points, &normal );
            create_two_orthogonal_vectors( &normal, &hor, &vert );

            for_less( n, 0, n_neighbours[point] )
            {
                neigh = neighbours[point][n];
                SUB_POINTS( offset, points[neigh], points[point] );
                x_flat[n] = DOT_VECTORS( offset, hor );
                y_flat[n] = DOT_VECTORS( offset, vert );
                z_flat[n] = 0.0;
            }

            if( !get_prediction_weights_3d( 0.0, 0.0, 0.0,
                                            n_neighbours[point],
                                            x_flat, y_flat, z_flat,
                                            weights[0], weights[1], weights[2]))

            {
                print_error( "Error in interpolation weights.\n" );
            }

            for_less( n, 0, n_neighbours[point] )
                centroid_weights[ind+n] = (dtype) weights[0][n][n];
#endif

            ind += n_neighbours[point];
        }

        for_less( dim1, 0, 3 )
        for_less( dim2, 0, 3 )
            FREE( weights[dim1][dim2] );
        FREE( x_flat );
        FREE( y_flat );
        FREE( z_flat );
        FREE( plane_points );
    }

    ALLOC( distances, total_edges );
    ind = 0;

    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            if( neighbours[point][n] > point )
            {
                distances[ind] = (dtype) sq_distance_between_points(
                                       &points[point],
                                       &points[neighbours[point][n]] );
                ++ind;
            }
        }
    }

    n_parameters = 3 * n_points;
    if( sphere_weight > 0.0 )
        ++n_parameters;
    ALLOC( parameters, n_parameters );
    ALLOC( g, n_parameters );
    ALLOC( h, n_parameters );
    ALLOC( xi, n_parameters );
    ALLOC( unit_dir, n_parameters );

    if( getenv( "SHAKE" ) == NULL ||
        sscanf( getenv( "SHAKE" ), "%d", &shake_every) != 1 )
        shake_every = 0;

    if( getenv( "SHAKE_RATIO" ) == NULL ||
        sscanf( getenv( "SHAKE_RATIO" ), "%lf", &shake_ratio) != 1 )
        shake_ratio = 0.3;

    for_less( point, 0, n_points )
    {
        parameters[IJ(point,0,3)] = (dtype) Point_x(init_points[point] );
        parameters[IJ(point,1,3)] = (dtype) Point_y(init_points[point] );
        parameters[IJ(point,2,3)] = (dtype) Point_z(init_points[point] );
    }

    if( init_supplied )
        FREE( init_points );

    if( sphere_weight > 0.0 )
        parameters[n_parameters-1] = (dtype) radius;

    sphere_weight *= (Real) total_edges / (Real) n_points;
    centroid_weight *= (Real) total_edges / (Real) n_points;

    fit = evaluate_fit( n_parameters, parameters, distances,
                        n_neighbours, neighbours,
                        centroid_weight, centroid_weights, sphere_weight );

    print( "Initial  %g\n", fit );
    (void) flush_file( stdout );

    evaluate_fit_derivative( n_parameters, parameters, distances,
                             n_neighbours, neighbours, centroid_weight,
                             centroid_weights, sphere_weight, xi );

    for_less( p, 0, n_parameters )
    {
        g[p] = -xi[p];
        h[p] = g[p];
        xi[p] = g[p];
    }

    update_rate = 1;
    last_update_time = current_cpu_seconds();
    n_same_size = 0;

    for_less( iter, 0, n_iters )
    {
        len = 0.0;
        for_less( p, 0, n_parameters )
            len += (Real) xi[p] * (Real) xi[p];

        len = sqrt( len );
        for_less( p, 0, n_parameters )
            unit_dir[p] = (dtype) ((Real) xi[p] / len);

        if( getenv( "NO_MINIMIZE" ) == NULL )
        minimize_along_line( n_parameters, parameters, unit_dir, distances,
                             n_neighbours, neighbours,
                             centroid_weight, centroid_weights, sphere_weight );

        if( shake_every > 0 && (iter % shake_every) == 0 )
        {
            Real  xc, yc, zc, test_fit;
            int   p_index, n_index, n_loop;

            fit = evaluate_fit( n_parameters, parameters, distances,
                                n_neighbours, neighbours, centroid_weight,
                                centroid_weights, sphere_weight );

            n_loop = 0;
            do
            {
                for_less( point, 0, n_points )
                {
                    xc = 0.0;
                    yc = 0.0;
                    zc = 0.0;
                    for_less( n, 0, n_neighbours[point] )
                    {
                        n_index = 3 * neighbours[point][n];
                        xc += (Real) parameters[n_index+0];
                        yc += (Real) parameters[n_index+1];
                        zc += (Real) parameters[n_index+2];
                    }
                    xc /= (Real) n_neighbours[point];
                    yc /= (Real) n_neighbours[point];
                    zc /= (Real) n_neighbours[point];

                    p_index = 3 * point;
                    xi[p_index+0] = (dtype) ((Real) parameters[p_index+0] *
                           (1.0 - shake_ratio) + shake_ratio * xc);
                    xi[p_index+1] = (dtype) ((Real) parameters[p_index+1] *
                           (1.0 - shake_ratio) + shake_ratio * yc);
                    xi[p_index+2] = (dtype) ((Real) parameters[p_index+2] *
                           (1.0 - shake_ratio) + shake_ratio * zc);
                }

                test_fit = evaluate_fit( n_parameters, xi, distances,
                                    n_neighbours, neighbours, centroid_weight,
                                    centroid_weights, sphere_weight );

                if( test_fit >= fit )
                {
                    if( shake_ratio > 1.0e-20 )
                        shake_ratio /= 2.0;
                    else
                        break;
                }
                ++n_loop;
            }
            while( test_fit >= fit );

            if( test_fit <= fit )
            {
                for_less( p, 0, n_parameters )
                    parameters[p] = xi[p];
            }

            if( n_loop == 1 )
            {
                ++n_same_size;
                if( n_same_size == 5 )
                {
                    shake_ratio *= 2.0;
                    n_same_size = 0;
                }
            }
            else
                n_same_size = 0;
        }

        if( ((iter+1) % update_rate) == 0 || iter == n_iters - 1 )
        {
            fit = evaluate_fit( n_parameters, parameters, distances,
                                n_neighbours, neighbours, centroid_weight,
                                centroid_weights, sphere_weight );

            print( "%d: %g %g", iter+1, fit, shake_ratio );
            if( sphere_weight > 0.0 )
                print( "\t Radius: %g", (Real) parameters[n_parameters-1] );
            print( "\n" );

            (void) flush_file( stdout );
            current_time = current_cpu_seconds();
            if( current_time - last_update_time < 1.0 )
                update_rate *= 10;

            last_update_time = current_time;
        }

        evaluate_fit_derivative( n_parameters, parameters, distances,
                                 n_neighbours, neighbours, centroid_weight,
                                 centroid_weights, sphere_weight, xi );

        gg = 0.0;
        dgg = 0.0;
        for_less( p, 0, n_parameters )
        {
            gg += (Real) g[p] * (Real) g[p];
            dgg += ((Real) xi[p] + (Real) g[p]) * (Real) xi[p];
/*
            dgg += ((Real) xi[p] * (Real) xi[p];
*/
        }

        if( len == 0.0 )
            break;

        gam = dgg / gg;

        for_less( p, 0, n_parameters )
        {
            g[p] = -xi[p];
            h[p] = (dtype) ((Real) g[p] + gam * (Real) h[p]);
            xi[p] = h[p];
        }
    }

    for_less( point, 0, n_points )
    {
        fill_Point( points[point],
                    parameters[IJ(point,0,3)],
                    parameters[IJ(point,1,3)],
                    parameters[IJ(point,2,3)] );
    }

    if( centroid_weight > 0.0 )
        FREE( centroid_weights );
    FREE( distances );
    FREE( parameters );
    FREE( xi );
    FREE( g );
    FREE( h );
    FREE( unit_dir );
}
