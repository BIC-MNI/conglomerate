#include  <internal_volume_io.h>
#include  <bicpl.h>

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
    int                  *n_neighbours, **neighbours;
    File_formats         format;
    object_struct        **object_list, **i_object_list;
    polygons_struct      *polygons, *init_polygons;
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
    ALLOC( polygons->points, 1 );
    delete_object_list( 1, object_list );

    flatten_polygons( polygons->n_points, points, n_neighbours, neighbours,
                      init_points, centroid_weight,
                      sphere_weight, n_iters );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     NULL, NULL );

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
    Real    sphere_weight )
{
    int    p, n_points, ind, n, neigh;
    Real   fit, dx, dy, dz, dist, diff, act_dist, radius;
    Real   xc, yc, zc, x, y, z;

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
        for_less( p, 0, n_points )
        {
            xc = 0.0;
            yc = 0.0;
            zc = 0.0;
            for_less( n, 0, n_neighbours[p] )
            {
                neigh = neighbours[p][n];
                xc += (Real) parameters[IJ(neigh,0,3)];
                yc += (Real) parameters[IJ(neigh,1,3)];
                zc += (Real) parameters[IJ(neigh,2,3)];
            }

            xc /= (Real) n_neighbours[p];
            yc /= (Real) n_neighbours[p];
            zc /= (Real) n_neighbours[p];

            x = (Real) parameters[IJ(p,0,3)];
            y = (Real) parameters[IJ(p,1,3)];
            z = (Real) parameters[IJ(p,2,3)];

            dx = x - xc;
            dy = y - yc;
            dz = z - zc;

            diff = dx * dx + dy * dy + dz * dz;

            fit += centroid_weight * diff * diff;
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
    Real     sphere_weight,
    dtype    deriv[] )
{
    int    p, n_points, ind, n, neigh, p_index, n_index;
    Real   dx, dy, dz, dist, diff, act_dist, radius, factor;
    Real   x1, y1, z1, x2, y2, z2, weight;
    Real   xc, yc, zc;
    Real   x_factor, y_factor, z_factor, x_diff, y_diff, z_diff;

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

        if( sphere_weight > 0.0 )
        {
            act_dist = x1 * x1 + y1 * y1 + z1 * z1;
            diff = act_dist - radius * radius;
            weight = 4.0 * sphere_weight * diff;
            deriv[p_index+0] += (dtype) (weight * x1);
            deriv[p_index+1] += (dtype) (weight * y1);
            deriv[p_index+2] += (dtype) (weight * z1);
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

            x2 = (Real) parameters[IJ(neigh,0,3)];
            y2 = (Real) parameters[IJ(neigh,1,3)];
            z2 = (Real) parameters[IJ(neigh,2,3)];
            dx = x1 - x2;
            dy = y1 - y2;
            dz = z1 - z2;
            act_dist = dx * dx + dy * dy + dz * dz;
            factor = 2.0 * (act_dist - dist);
            deriv[p_index+0] += (dtype) (dx * factor);
            deriv[p_index+1] += (dtype) (dy * factor);
            deriv[p_index+2] += (dtype) (dz * factor);
            deriv[n_index+0] += (dtype) (-dx * factor);
            deriv[n_index+1] += (dtype) (-dy * factor);
            deriv[n_index+2] += (dtype) (-dz * factor);
        }
    }

    if( centroid_weight > 0.0 )
    {
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
                xc += (Real) parameters[n_index+0];
                yc += (Real) parameters[n_index+1];
                zc += (Real) parameters[n_index+2];
            }

            weight = 1.0 / (Real) n_neighbours[p];
            xc *= weight;
            yc *= weight;
            zc *= weight;
            x_diff = xc - x1;
            x_diff = x_diff * x_diff * x_diff;
            y_diff = yc - y1;
            y_diff = y_diff * y_diff * y_diff;
            z_diff = zc - z1;
            z_diff = z_diff * z_diff * z_diff;
            x_factor = centroid_weight * 4.0 * x_diff;
            y_factor = centroid_weight * 4.0 * y_diff;
            z_factor = centroid_weight * 4.0 * z_diff;

            deriv[p_index+0] += (dtype) (-x_factor);
            deriv[p_index+1] += (dtype) (-y_factor);
            deriv[p_index+2] += (dtype) (-z_factor);
            x_factor *= weight;
            y_factor *= weight;
            z_factor *= weight;

            for_less( n, 0, n_neighbours[p] )
            {
                neigh = neighbours[p][n];
                n_index = IJ(neigh,0,3);
                deriv[n_index+0] += (dtype) x_factor;
                deriv[n_index+1] += (dtype) y_factor;
                deriv[n_index+2] += (dtype) z_factor;
            }
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
    Real    sphere_weight,
    Real    coefs[] )
{
    int    p, n_points, ind, n, neigh, p_index, n_index;
    Real   dx, dy, dz, dr, dist, radius;
    Real   dx1, dy1, dz1, x1, y1, z1;
    Real   x, y, z;
    Real   a, b, c, ax, ay, az, bx, by, bz, weight;
    Real   line_coefs[3], s_coefs[5];

    for_less( p, 0, 5 )
        coefs[p] = 0.0;

    n_points = n_parameters / 3;

    if( sphere_weight > 0.0 )
    {
        radius = (Real) parameters[n_parameters-1];
        dr = (Real) delta[n_parameters-1];
    }

    if( sphere_weight > 0.0 )
        sphere_weight = sqrt( sphere_weight );

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

        line_coefs[0] = x1 * x1 + y1 * y1 + z1 * z1 - radius * radius;
        line_coefs[1] = 2.0 * (x1 * dx1 + y1 * dy1 + z1 * dz1 - radius * dr);
        line_coefs[2] = dx1 * dx1 + dy1 * dy1 + dz1 * dz1 - dr * dr;
        line_coefs[0] *= sphere_weight;
        line_coefs[1] *= sphere_weight;
        line_coefs[2] *= sphere_weight;

        coefs[0] += line_coefs[0] * line_coefs[0];
        coefs[1] += 2.0 * line_coefs[1] * line_coefs[0];
        coefs[2] += 2.0 * line_coefs[2] * line_coefs[0] +
                          line_coefs[1] * line_coefs[1];
        coefs[3] += 2.0 * line_coefs[2] * line_coefs[1];
        coefs[4] += line_coefs[2] * line_coefs[2];

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

            line_coefs[0] = x * x + y * y + z * z - dist;
            line_coefs[1] = 2.0 * (x * dx + y * dy + z * dz);
            line_coefs[2] = dx * dx + dy * dy + dz * dz;

            coefs[0] += line_coefs[0] * line_coefs[0];
            coefs[1] += 2.0 * line_coefs[1] * line_coefs[0];
            coefs[2] += 2.0 * line_coefs[2] * line_coefs[0] +
                              line_coefs[1] * line_coefs[1];
            coefs[3] += 2.0 * line_coefs[2] * line_coefs[1];
            coefs[4] += line_coefs[2] * line_coefs[2];
        }
    }

    if( centroid_weight > 0.0 )
    {
        for_less( p, 0, 5 )
            s_coefs[p] = 0.0;

        for_less( p, 0, n_points )
        {
            p_index = IJ( p, 0, 3 );

            ax = 0.0;
            ay = 0.0;
            az = 0.0;
            bx = 0.0;
            by = 0.0;
            bz = 0.0;

            for_less( n, 0, n_neighbours[p] )
            {
                neigh = neighbours[p][n];
                n_index = IJ(neigh,0,3);
                ax += (Real) delta[n_index+0];
                ay += (Real) delta[n_index+1];
                az += (Real) delta[n_index+2];
                bx += (Real) parameters[n_index+0];
                by += (Real) parameters[n_index+1];
                bz += (Real) parameters[n_index+2];
            }

            weight = 1.0 / (Real) n_neighbours[p];

            ax *= weight;
            ay *= weight;
            az *= weight;
            bx *= weight;
            by *= weight;
            bz *= weight;

            ax += (Real) -delta[p_index+0];
            ay += (Real) -delta[p_index+1];
            az += (Real) -delta[p_index+2];
            bx += (Real) -parameters[p_index+0];
            by += (Real) -parameters[p_index+1];
            bz += (Real) -parameters[p_index+2];

            a = ax * ax;
            b = 2.0 * ax * bx;
            c = bx * bx;

            s_coefs[0] += c * c;
            s_coefs[1] += 2.0 * b * c;
            s_coefs[2] += 2.0 * a * c + b * b;
            s_coefs[3] += 2.0 * a * b;
            s_coefs[4] += a * a;

            a = ay * ay;
            b = 2.0 * ay * by;
            c = by * by;

            s_coefs[0] += c * c;
            s_coefs[1] += 2.0 * b * c;
            s_coefs[2] += 2.0 * a * c + b * b;
            s_coefs[3] += 2.0 * a * b;
            s_coefs[4] += a * a;

            a = az * az;
            b = 2.0 * az * bz;
            c = bz * bz;

            s_coefs[0] += c * c;
            s_coefs[1] += 2.0 * b * c;
            s_coefs[2] += 2.0 * a * c + b * b;
            s_coefs[3] += 2.0 * a * b;
            s_coefs[4] += a * a;
        }

        for_less( p, 0, 5 )
            coefs[p] += centroid_weight * s_coefs[p];
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
    Real    sphere_weight )
{
    int    p, s, n_solutions, best_index;
    Real   coefs[5], deriv[4], *test, t, fit, best_fit, solutions[3];
/*
    Real   test_fit;
*/

    ALLOC( test, n_parameters );

    evaluate_fit_along_line( n_parameters, parameters, delta, distances,
                             n_neighbours, neighbours, centroid_weight,
                             sphere_weight, coefs );

    for_less( p, 0, 4 )
        deriv[p] = (Real) (p+1) * coefs[p+1];

    n_solutions = solve_cubic( deriv[3], deriv[2], deriv[1], deriv[0],
                               solutions );

    if( n_solutions == 0 )
        print( "N solutions = 0\n" );

    best_fit = coefs[0];
    best_index = -1;

/*
    test_fit = evaluate_fit( n_parameters, parameters, distances,
                             n_neighbours, neighbours, sphere_weight );
    print( "## %g %g\n", coefs[0], test_fit );
*/

    for_less( s, 0, n_solutions )
    {
        t = solutions[s];

/*
        for_less( p, 0, n_parameters )
            test[p] = parameters[p] + t * delta[p];
*/

        fit = coefs[0] + t * (coefs[1] + t * (coefs[2] + t * (coefs[3] +
              t * coefs[4])));

/*
        test_fit = evaluate_fit( n_parameters, test, distances,
                                 n_neighbours, neighbours, sphere_weight );

        print( "%g %g\n", fit, test_fit );
*/

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
    int              p, n, n1, point;
    int              n_parameters, total_neighbours;
    Real             gg, dgg, gam, current_time, last_update_time;
    Real             fit;
    Real             len, radius;
    Point            centroid;
    int              iter, ind, update_rate;
    dtype            *g, *h, *xi, *parameters, *unit_dir, *distances;
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
    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
            if( neighbours[point][n] > point )
                ++total_neighbours;
    }

    ALLOC( distances, total_neighbours );
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

    sphere_weight *= (Real) total_neighbours / (Real) n_points;
    centroid_weight *= (Real) total_neighbours / (Real) n_points;

    fit = evaluate_fit( n_parameters, parameters, distances,
                        n_neighbours, neighbours,
                        centroid_weight, sphere_weight );

    print( "Initial  %g\n", fit );
    (void) flush_file( stdout );

    evaluate_fit_derivative( n_parameters, parameters, distances,
                             n_neighbours, neighbours, centroid_weight,
                             sphere_weight, xi );

    for_less( p, 0, n_parameters )
    {
        g[p] = -xi[p];
        h[p] = g[p];
        xi[p] = g[p];
    }

    update_rate = 1;
    last_update_time = current_cpu_seconds();

    for_less( iter, 0, n_iters )
    {
        len = 0.0;
        for_less( p, 0, n_parameters )
            len += (Real) xi[p] * (Real) xi[p];

        len = sqrt( len );
        for_less( p, 0, n_parameters )
            unit_dir[p] = (dtype) ((Real) xi[p] / len);

        minimize_along_line( n_parameters, parameters, unit_dir, distances,
                             n_neighbours, neighbours,
                             centroid_weight, sphere_weight );

        if( ((iter+1) % update_rate) == 0 || iter == n_iters - 1 )
        {
            fit = evaluate_fit( n_parameters, parameters, distances,
                                n_neighbours, neighbours, centroid_weight,
                                sphere_weight );

            print( "%d: %g", iter+1, fit );
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
                                 sphere_weight, xi );

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

    FREE( distances );
    FREE( parameters );
    FREE( xi );
    FREE( g );
    FREE( h );
    FREE( unit_dir );
}
