#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  TOLERANCE         1.0e-7

typedef  float  dtype;

private  void  flatten_polygons(
    int              n_points,
    Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    Point            init_points[],
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename, initial_filename;
    int                  n_objects, n_i_objects, n_iters, poly;
    int                  *n_neighbours, **neighbours, n_points;
    File_formats         format;
    object_struct        **object_list, **i_object_list;
    polygons_struct      *polygons, *init_polygons, p;
    Point                *init_points, *points;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj [cw] [sw] [n_iters]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_iters );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    for_less( poly, 0, polygons->n_items )
    {
        if( GET_OBJECT_SIZE( *polygons, poly ) != 3 )
            break;
    }

    if( poly < polygons->n_items )
    {
        print_error( "Must be triangulated polygon mesh.\n" );
        return( 1 );
    }

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
                      init_points, n_iters );

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

static  Real  global_radius;

private  Real  evaluate_fit(
    int     n_parameters,
    dtype   parameters[],
    int     n_neighbours[],
    int     *neighbours[] )
{
    int     p, n_points, max_neighbours, n, neigh, p_index, n_index;
    int     next_n;
    Real    fit;
    dtype   n_len, radius;
    dtype   nx, ny, nz, x1, y1, z1, x2, y2, z2;
    dtype   dx, dy, dz, x3, y3, z3;
    dtype   *x_centroids, *y_centroids, *z_centroids;
    dtype   *x_normals, *y_normals, *z_normals;

    fit = 0.0;

    radius = (dtype) global_radius;

    n_points = n_parameters / 3;

    max_neighbours = 0;
    for_less( p, 0, n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[p] );

    ALLOC( x_centroids, max_neighbours );
    ALLOC( y_centroids, max_neighbours );
    ALLOC( z_centroids, max_neighbours );
    ALLOC( x_normals, max_neighbours );
    ALLOC( y_normals, max_neighbours );
    ALLOC( z_normals, max_neighbours );

    for_less( p, 0, n_points )
    {
        p_index = IJ(p,0,3);
        x1 = parameters[p_index+0];
        y1 = parameters[p_index+1];
        z1 = parameters[p_index+2];

        neigh = neighbours[p][n_neighbours[p]-1];
        n_index = IJ(neigh,0,3);
        x3 = parameters[n_index+0];
        y3 = parameters[n_index+1];
        z3 = parameters[n_index+2];

        for_less( n, 0, n_neighbours[p] )
        {
            x2 = x3;
            y2 = y3;
            z2 = z3;

            neigh = neighbours[p][n];
            n_index = IJ(neigh,0,3);
            x3 = parameters[n_index+0];
            y3 = parameters[n_index+1];
            z3 = parameters[n_index+2];

            nx = (y2 - y1) * (z3 - z1) - (y3 - y1) * (z2 - z1);
            ny = (z2 - z1) * (x3 - x1) - (z3 - z1) * (x2 - x1);
            nz = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);

            n_len = nx * nx + ny * ny + nz * nz;
            if( n_len == 0.0f )
                continue;

            n_len = sqrtf( n_len );
            x_normals[n] = nx / n_len;
            y_normals[n] = ny / n_len;
            z_normals[n] = nz / n_len;

            x_centroids[n] = (x1 + x2 + x3) / 3.0f;
            y_centroids[n] = (y1 + y2 + y3) / 3.0f;
            z_centroids[n] = (z1 + z2 + z3) / 3.0f;
        }

        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            if( neigh < p )
                continue;

            next_n = (n+1)%n_neighbours[p];

            dx = (x_centroids[n] - x_normals[n] * radius) -
                 (x_centroids[next_n] - x_normals[next_n] * radius);
            dy = (y_centroids[n] - y_normals[n] * radius) -
                 (y_centroids[next_n] - y_normals[next_n] * radius);
            dz = (z_centroids[n] - z_normals[n] * radius) -
                 (z_centroids[next_n] - z_normals[next_n] * radius);

            fit += (Real) (dx * dx + dy * dy + dz * dz);
        }
    }

    FREE( x_centroids );
    FREE( y_centroids );
    FREE( z_centroids );
    FREE( x_normals );
    FREE( y_normals );
    FREE( z_normals );

    return( fit );
}

private  void  evaluate_fit_derivative(
    int      n_parameters,
    dtype    parameters[],
    int      n_neighbours[],
    int      *neighbours[],
    dtype    deriv[] )
{
    int     p, n_points, max_neighbours, n, neigh, p_index, n_index;
    int     next_n, prev_n, next_index, prev_index;
    dtype   n_len, radius, *lens, len1, len2;
    dtype   nx, ny, nz, x1, y1, z1, x2, y2, z2;
    dtype   dx, dy, dz, x3, y3, z3, x4, y4, z4;
    dtype   *x_deltas, *y_deltas, *z_deltas;
    dtype   *x_normals, *y_normals, *z_normals;

    for_less( p, 0, n_parameters )
        deriv[p] = (dtype) 0.0;

    radius = (dtype) global_radius;

    n_points = n_parameters / 3;

    max_neighbours = 0;
    for_less( p, 0, n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[p] );

    ALLOC( x_deltas, max_neighbours );
    ALLOC( y_deltas, max_neighbours );
    ALLOC( z_deltas, max_neighbours );
    ALLOC( x_normals, max_neighbours );
    ALLOC( y_normals, max_neighbours );
    ALLOC( z_normals, max_neighbours );
    ALLOC( lens, max_neighbours );

    for_less( p, 0, n_points )
    {
        p_index = IJ(p,0,3);
        x1 = parameters[p_index+0];
        y1 = parameters[p_index+1];
        z1 = parameters[p_index+2];

        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            n_index = IJ(neigh,0,3);
            x_deltas[n] = parameters[n_index+0] - x1;
            y_deltas[n] = parameters[n_index+1] - y1;
            z_deltas[n] = parameters[n_index+2] - z1;
        }

        for_less( n, 0, n_neighbours[p] )
        {
            prev_n = (n - 1 + n_neighbours[p]) % n_neighbours[p];

            nx = y_deltas[prev_n]*z_deltas[n] - y_deltas[n]*z_deltas[prev_n];
            ny = z_deltas[prev_n]*x_deltas[n] - z_deltas[n]*x_deltas[prev_n];
            nz = x_deltas[prev_n]*y_deltas[n] - x_deltas[n]*y_deltas[prev_n];

            n_len = nx * nx + ny * ny + nz * nz;
            lens[n] = n_len;
            if( n_len == 0.0f )
                continue;

            n_len = sqrtf( n_len );
            lens[n] = n_len;
            x_normals[n] = nx / n_len;
            y_normals[n] = ny / n_len;
            z_normals[n] = nz / n_len;
        }

        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            if( neigh < p )
                continue;

            n_index = IJ( neigh, 0, 3 );
            x3 = parameters[n_index+0];
            y3 = parameters[n_index+1];
            z3 = parameters[n_index+2];

            prev_n = (n-1+n_neighbours[p])%n_neighbours[p];
            prev_index = IJ( neighbours[p][prev_n], 0, 3 );
            x2 = parameters[prev_index+0];
            y2 = parameters[prev_index+1];
            z2 = parameters[prev_index+2];

            next_n = (n+1)%n_neighbours[p];
            next_index = IJ( neighbours[p][next_n], 0, 3 );
            x4 = parameters[next_index+0];
            y4 = parameters[next_index+1];
            z4 = parameters[next_index+2];

            dx = (x2/3.0f - x_normals[n] * radius) -
                 (x4/3.0f - x_normals[next_n] * radius);
            dy = (y2/3.0f - y_normals[n] * radius) -
                 (y4/3.0f - y_normals[next_n] * radius);
            dz = (z2/3.0f - z_normals[n] * radius) -
                 (z4/3.0f - z_normals[next_n] * radius);

            dx *= 2.0f;
            dy *= 2.0f;
            dz *= 2.0f;

            len1 = lens[n];
            len2 = lens[next_n];

            deriv[p_index+0] += dz * (radius / len1 * (y3 - y2) +
                                      radius / len2 * (y3 - y4)) +
                                dy * (radius / len1 * (z2 - z3) +
                                      radius / len2 * (z4 - z3));
            deriv[p_index+1] += dx * (radius / len1 * (z3 - z2) +
                                      radius / len2 * (z3 - z4)) +
                                dz * (radius / len1 * (x2 - x3) +
                                      radius / len2 * (x4 - x3));
            deriv[p_index+2] += dy * (radius / len1 * (x3 - x2) +
                                      radius / len2 * (x3 - x4)) +
                                dx * (radius / len1 * (y2 - y3) +
                                      radius / len2 * (y4 - y3));

            deriv[prev_index+0] += dz * radius / len1 * (y1 - y3) +
                                   dy * radius / len1 * (z3 - z1);
            deriv[prev_index+1] += dx * radius / len1 * (z1 - z3) +
                                   dz * radius / len1 * (x3 - x1);
            deriv[prev_index+2] += dy * radius / len1 * (x1 - x3) +
                                   dx * radius / len1 * (y3 - y1);

            deriv[n_index+0] += dz * (radius / len1 * (y2 - y1) +
                                      radius / len2 * (y4 - y1)) +
                                dy * (radius / len1 * (z1 - z2) +
                                      radius / len2 * (z1 - z4));
            deriv[n_index+1] += dx * (radius / len1 * (z2 - z1) +
                                      radius / len2 * (z4 - z1)) +
                                dz * (radius / len1 * (x1 - x2) +
                                      radius / len2 * (x1 - x4));
            deriv[n_index+2] += dy * (radius / len1 * (x2 - x1) +
                                      radius / len2 * (x4 - x1)) +
                                dx * (radius / len1 * (y1 - y2) +
                                      radius / len2 * (y1 - y4));

            deriv[next_index+0] += dz * radius / len2 * (y1 - y3) +
                                   dy * radius / len2 * (z3 - z1);
            deriv[next_index+1] += dx * radius / len2 * (z1 - z3) +
                                   dz * radius / len2 * (x3 - x1);
            deriv[next_index+2] += dy * radius / len2 * (x1 - x3) +
                                   dx * radius / len2 * (y3 - y1);

            dx *= 1.0f / 3.0f;
            dy *= 1.0f / 3.0f;
            dz *= 1.0f / 3.0f;
            deriv[prev_index+0] += dx;
            deriv[prev_index+1] += dy;
            deriv[prev_index+2] += dz;
            deriv[next_index+0] += -dx;
            deriv[next_index+1] += -dy;
            deriv[next_index+2] += -dz;
        }
    }

    FREE( x_deltas );
    FREE( y_deltas );
    FREE( z_deltas );
    FREE( x_normals );
    FREE( y_normals );
    FREE( z_normals );
    FREE( lens );
}

private  Real  evaluate_fit_along_line(
    int     n_parameters,
    dtype   parameters[],
    dtype   line_dir[],
    dtype   buffer[],
    Real    dist,
    int     n_neighbours[],
    int     *neighbours[] )
{
    int     p;
    Real    fit;
    
    for_less( p, 0, n_parameters )
        buffer[p] = (float) ( (Real) parameters[p] + dist * (Real) line_dir[p]);

    fit = evaluate_fit( n_parameters, buffer, n_neighbours, neighbours );

    return( fit );
}

#define  GOLDEN_RATIO   0.618034

private  Real  minimize_along_line(
    Real    current_fit,
    int     n_parameters,
    dtype   parameters[],
    dtype   line_dir[],
    int     n_neighbours[],
    int     *neighbours[],
    BOOLEAN *changed )
{
    int      p, n_iters;
    Real     t0, t1, t2, f0, f1, f2, t_next, f_next;
    float    *test_parameters;

    *changed = FALSE;

    ALLOC( test_parameters, n_parameters );

    t1 = 0.0;
    f1 = current_fit;

    t0 = -0.01 / 2.0;
    do
    {
        t0 *= 2.0;

        if( t0 < -1e30 )
            return( current_fit );

        f0 = evaluate_fit_along_line( n_parameters, parameters, line_dir,
                                      test_parameters, t0,
                                      n_neighbours, neighbours );
    }
    while( f0 <= f1 );

    t2 = 0.01 / 2.0;
    do
    {
        t2 *= 2.0;

        if( t2 > 1e30 )
            return( current_fit );

        f2 = evaluate_fit_along_line( n_parameters, parameters, line_dir,
                                      test_parameters, t2,
                                      n_neighbours, neighbours );
    }
    while( f2 <= f1 );

    n_iters = 0;
    do
    {
        t_next = t0 + (t1 - t0) * GOLDEN_RATIO;

        f_next = evaluate_fit_along_line( n_parameters, parameters, line_dir,
                                          test_parameters, t_next,
                                          n_neighbours, neighbours );

/*
        print( "%g  %g  %g  %g\n", t0, t_next, t1, t2 );
        print( "%g  %g  %g  %g\n\n", f0, f_next, f1, f2 );
*/

        if( f_next <= f1 )
        {
            t2 = t1;
            f2 = f1;
            t1 = t_next;
            f1 = f_next;
        }
        else
        {
            t0 = t2;
            f0 = f2;
            t2 = t_next;
            f2 = f_next;
        }
        ++n_iters;
    }
    while( FABS( t2 - t0 ) > TOLERANCE );

    if( t1 != 0.0 )
    {
        *changed = TRUE;
        current_fit = f1;
        for_less( p, 0, n_parameters )
            parameters[p] += (float) (t1 * (Real) line_dir[p]);
    }

    FREE( test_parameters );

    return( current_fit );
}


private  void  flatten_polygons(
    int              n_points,
    Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    Point            init_points[],
    int              n_iters )
{
    int              p, point, max_neighbours, n_parameters;
    Real             gg, dgg, gam, current_time, last_update_time, fit;
    Real             len, radius, step;
    Point            tri[3], centroid, c;
    int              iter, update_rate, count, neigh, next_neigh, n;
    dtype            *g, *h, *xi, *parameters, *unit_dir;
    BOOLEAN          init_supplied, changed, debug, testing;

    debug = getenv( "DEBUG" ) != NULL;

    init_supplied = (init_points != NULL);
    if( !init_supplied )
        init_points = points;

    count = 0;
    fill_Point( centroid, 0.0, 0.0, 0.0 );
    for_less( point, 0, n_points )
    {
        tri[0] = points[point];
        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            next_neigh = neighbours[point][(n+1)%n_neighbours[point]];
            if( point > neigh || point > next_neigh )
                continue;

            tri[1] = points[neigh];
            tri[2] = points[next_neigh];
            get_points_centroid( 3, tri, &c );
            ADD_POINTS( centroid, centroid, c );
            ++count;
        }
    }

    SCALE_POINT( centroid, centroid, 1.0 / (Real) count );

    radius = 0.0;
    for_less( point, 0, n_points )
    {
        tri[0] = points[point];
        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            next_neigh = neighbours[point][(n+1)%n_neighbours[point]];
            if( point > neigh || point > next_neigh )
                continue;

            tri[1] = points[neigh];
            tri[2] = points[next_neigh];
            get_points_centroid( 3, tri, &c );
            radius += distance_between_points( &c, &centroid );
        }
    }

    radius /= (Real) count;

    global_radius = radius;

    max_neighbours = 0;
    for_less( p, 0, n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[p] );

    n_parameters = 3 * n_points;

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

    fit = evaluate_fit( n_parameters, parameters, n_neighbours, neighbours );

    print( "Initial  %g\n", fit );
    (void) flush_file( stdout );

    evaluate_fit_derivative( n_parameters, parameters,
                             n_neighbours, neighbours, xi );

    if( getenv( "STEP" ) != NULL && sscanf( getenv("STEP"), "%lf", &step ) == 1)
        testing = TRUE;
    else
        testing = FALSE;

    if( testing )
    {
        dtype  save;
        Real   f1, f2, test_deriv;

        for_less( p, 0, n_parameters )
        {
            save = parameters[p];
            parameters[p] = (dtype) ((Real) save - step);
            f1 = evaluate_fit( n_parameters, parameters,
                               n_neighbours, neighbours );
            parameters[p] = (dtype) ((Real) save + step);
            f2 = evaluate_fit( n_parameters, parameters,
                               n_neighbours, neighbours );
            parameters[p] = save;

            test_deriv = (f2 - f1) / 2.0 / step;


/*
            if( !numerically_close( test_deriv, (Real) xi[p] , 1.0e-5 ) )
                print( "Derivs mismatch %d d%c:  %g  %g\n",
                       p / 3, "XYZ"[p%3], test_deriv, xi[p] );
*/

            xi[p] = (dtype) test_deriv;
        }
    }


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

        if( debug )
        {
            for_less( p, 0, n_parameters )
                unit_dir[p] = 0.0f;
            unit_dir[iter % n_parameters] = 1.0f;
        }

        fit = minimize_along_line( fit, n_parameters, parameters, unit_dir,
                                   n_neighbours, neighbours, &changed );

        if( !debug )
        {
            if( !changed )
                break;
        }

        if( ((iter+1) % update_rate) == 0 || iter == n_iters - 1 )
        {
            print( "%d: %g", iter+1, fit );
            print( "\t Radius: %g", radius );
            print( "\n" );

            (void) flush_file( stdout );
            current_time = current_cpu_seconds();
            if( current_time - last_update_time < 1.0 )
                update_rate *= 10;

            last_update_time = current_time;
        }

        evaluate_fit_derivative( n_parameters, parameters,
                                 n_neighbours, neighbours, xi );

        if( testing )
        {
            dtype  save;
            Real   f1, f2, test_deriv;

            for_less( p, 0, n_parameters )
            {
                save = parameters[p];
                parameters[p] = (dtype) ((Real) save - step);
                f1 = evaluate_fit( n_parameters, parameters,
                                   n_neighbours, neighbours );
                parameters[p] = (dtype) ((Real) save + step);
                f2 = evaluate_fit( n_parameters, parameters,
                                   n_neighbours, neighbours );
                parameters[p] = save;

                test_deriv = (f2 - f1) / 2.0 / step;

/*
                if( !numerically_close( test_deriv, (Real) xi[p] , 1.0e-5 ) )
                    print( "Derivs mismatch %d d%c:  %g  %g\n",
                           p / 3, "XYZ"[p%3], test_deriv, xi[p] );
*/
                xi[p] = (dtype) test_deriv;
            }
        }

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

        if( gg == 0.0 )
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

    FREE( parameters );
    FREE( xi );
    FREE( g );
    FREE( h );
    FREE( unit_dir );
}
