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
    Real             radius,
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename, initial_filename;
    int                  n_objects, n_i_objects, n_iters, poly;
    int                  *n_neighbours, **neighbours, n_points;
    Real                 radius;
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

    radius = sqrt( get_polygons_surface_area( polygons ) / 4.0 / PI );

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    points = polygons->points;
    n_points = polygons->n_points;
    ALLOC( polygons->points, 1 );
    delete_object_list( 1, object_list );

    flatten_polygons( n_points, points, n_neighbours, neighbours,
                      init_points, radius, n_iters );

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
    dtype   volumes[],
    int     n_neighbours[],
    int     *neighbours[] )
{
    int     p, n_points, n, neigh, p_index, ind;
    int     n_index, prev, next, prev_index, next_index;
    Real    fit, diff;
    dtype   nx, ny, nz, x1, y1, z1, x2, y2, z2, vol;
    dtype   x3, y3, z3, x4, y4, z4;

    fit = 0.0;

    n_points = n_parameters / 3;

    ind = 0;
    for_less( p, 0, n_points )
    {
        p_index = IJ(p,0,3);
        x1 = parameters[p_index+0];
        y1 = parameters[p_index+1];
        z1 = parameters[p_index+2];

        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            if( neigh < p )
                continue;

            n_index = IJ(neigh,0,3);
            x3 = parameters[n_index+0];
            y3 = parameters[n_index+1];
            z3 = parameters[n_index+2];

            prev = neighbours[p][(n-1+n_neighbours[p])%n_neighbours[p]];
            prev_index = IJ(prev,0,3);
            x2 = parameters[prev_index+0];
            y2 = parameters[prev_index+1];
            z2 = parameters[prev_index+2];

            next = neighbours[p][(n+1)%n_neighbours[p]];
            next_index = IJ(next,0,3);
            x4 = parameters[next_index+0];
            y4 = parameters[next_index+1];
            z4 = parameters[next_index+2];

            nx = (y2 - y1) * (z3 - z1) - (y3 - y1) * (z2 - z1);
            ny = (z2 - z1) * (x3 - x1) - (z3 - z1) * (x2 - x1);
            nz = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
            vol = nx * (x4 - x1) + ny * (y4 - y1) + nz * (z4 - z1);

            diff = (Real) (vol - volumes[ind]);
            ++ind;

            fit += diff * diff;
        }
    }

    return( fit );
}

private  void  evaluate_fit_derivative(
    int      n_parameters,
    dtype    parameters[],
    dtype    volumes[],
    int      n_neighbours[],
    int      *neighbours[],
    dtype    deriv[] )
{
    int     p, n_points, n, neigh, p_index, ind;
    int     n_index, prev, next, prev_index, next_index;
    dtype   nx, ny, nz, x1, y1, z1, x2, y2, z2, diff, d2, vol;
    dtype   x3, y3, z3, x4, y4, z4;

    for_less( p, 0, n_parameters )
        deriv[p] = (dtype) 0.0;

    n_points = n_parameters / 3;

    ind = 0;
    for_less( p, 0, n_points )
    {
        p_index = IJ(p,0,3);
        x1 = parameters[p_index+0];
        y1 = parameters[p_index+1];
        z1 = parameters[p_index+2];

        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            if( neigh < p )
                continue;

            n_index = IJ(neigh,0,3);
            x3 = parameters[n_index+0];
            y3 = parameters[n_index+1];
            z3 = parameters[n_index+2];

            prev = neighbours[p][(n-1+n_neighbours[p])%n_neighbours[p]];
            prev_index = IJ(prev,0,3);
            x2 = parameters[prev_index+0];
            y2 = parameters[prev_index+1];
            z2 = parameters[prev_index+2];

            next = neighbours[p][(n+1)%n_neighbours[p]];
            next_index = IJ(next,0,3);
            x4 = parameters[next_index+0];
            y4 = parameters[next_index+1];
            z4 = parameters[next_index+2];

            nx = (y2 - y1) * (z3 - z1) - (y3 - y1) * (z2 - z1);
            ny = (z2 - z1) * (x3 - x1) - (z3 - z1) * (x2 - x1);
            nz = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
            vol = nx * (x4 - x1) + ny * (y4 - y1) + nz * (z4 - z1);

            diff = vol - volumes[ind];
            ++ind;
            d2 = 2.0f * diff;

            deriv[p_index+0] += d2 *((y4-y1) * (z3-z2) + (z4-z1) * (y2-y3)-nx);
            deriv[p_index+1] += d2 *((z4-z1) * (x3-x2) + (x4-x1) * (z2-z3)-ny);
            deriv[p_index+2] += d2 *((x4-x1) * (y3-y2) + (y4-y1) * (x2-x3)-nz);

            deriv[prev_index+0] += d2 *((y4-y1) * (z1-z3) + (z4-z1) * (y3-y1));
            deriv[prev_index+1] += d2 *((z4-z1) * (x1-x3) + (x4-x1) * (z3-z1));
            deriv[prev_index+2] += d2 *((x4-x1) * (y1-y3) + (y4-y1) * (x3-x1));

            deriv[n_index+0] += d2 *((y4-y1) * (z2-z1) + (z4-z1) * (y1-y2));
            deriv[n_index+1] += d2 *((z4-z1) * (x2-x1) + (x4-x1) * (z1-z2));
            deriv[n_index+2] += d2 *((x4-x1) * (y2-y1) + (y4-y1) * (x1-x2));

            deriv[next_index+0] += d2 * nx;
            deriv[next_index+1] += d2 * ny;
            deriv[next_index+2] += d2 * nz;
        }
    }

}

private  Real  evaluate_fit_along_line(
    int     n_parameters,
    dtype   parameters[],
    dtype   line_dir[],
    dtype   buffer[],
    Real    dist,
    dtype   volumes[],
    int     n_neighbours[],
    int     *neighbours[] )
{
    int     p;
    Real    fit;
    
    for_less( p, 0, n_parameters )
        buffer[p] = (float) ( (Real) parameters[p] + dist * (Real) line_dir[p]);

    fit = evaluate_fit( n_parameters, buffer, volumes,
                        n_neighbours, neighbours );

    return( fit );
}

#define  GOLDEN_RATIO   0.618034

private  Real  minimize_along_line(
    Real    current_fit,
    int     n_parameters,
    dtype   parameters[],
    dtype   line_dir[],
    dtype   volumes[],
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
                                      volumes, n_neighbours, neighbours );
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
                                      volumes, n_neighbours, neighbours );
    }
    while( f2 <= f1 );

    n_iters = 0;
    do
    {
        t_next = t0 + (t1 - t0) * GOLDEN_RATIO;

        f_next = evaluate_fit_along_line( n_parameters, parameters, line_dir,
                                          test_parameters, t_next,
                                          volumes, n_neighbours, neighbours );

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
    Real             radius,
    int              n_iters )
{
    int              p, point, max_neighbours, n_parameters;
    Real             gg, dgg, gam, current_time, last_update_time, fit;
    Real             len, step;
    Point            p1, p2, p3, p4;
    Vector           offset, v12, v13, v14;
    int              iter, update_rate, neigh, n;
    int              n_edges, ind;
    dtype            *g, *h, *xi, *parameters, *unit_dir, *volumes;
    BOOLEAN          init_supplied, changed, debug, testing;

    debug = getenv( "DEBUG" ) != NULL;

    init_supplied = (init_points != NULL);
    if( !init_supplied )
        init_points = points;

    n_edges = 0;
    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            if( point < neigh )
                ++n_edges;
        }
    }

    ALLOC( volumes, n_edges );

    ind = 0;
    for_less( point, 0, n_points )
    {
        p1 = points[point];
        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            if( neigh < point )
                continue;

            p2 = points[neighbours[point][(n-1+n_neighbours[point]) %
                                          n_neighbours[point]]];
            p3 = points[neigh];
            p4 = points[neighbours[point][(n+1)%n_neighbours[point]]];

            SUB_POINTS( v12, p2, p1 );
            SUB_POINTS( v13, p3, p1 );
            SUB_POINTS( v14, p4, p1 );

            CROSS_VECTORS( offset, v12, v13 );
            volumes[ind] = (dtype) DOT_VECTORS( offset, v14 );
            ++ind;
        }
    }

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

    fit = evaluate_fit( n_parameters, parameters, volumes,
                        n_neighbours, neighbours );

    print( "Initial  %g\n", fit );
    (void) flush_file( stdout );

    evaluate_fit_derivative( n_parameters, parameters, volumes,
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
            f1 = evaluate_fit( n_parameters, parameters, volumes,
                               n_neighbours, neighbours );
            parameters[p] = (dtype) ((Real) save + step);
            f2 = evaluate_fit( n_parameters, parameters, volumes,
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
                                   volumes, n_neighbours, neighbours,
                                   &changed );

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

        evaluate_fit_derivative( n_parameters, parameters, volumes,
                                 n_neighbours, neighbours, xi );

        if( testing )
        {
            dtype  save;
            Real   f1, f2, test_deriv;

            for_less( p, 0, n_parameters )
            {
                save = parameters[p];
                parameters[p] = (dtype) ((Real) save - step);
                f1 = evaluate_fit( n_parameters, parameters, volumes,
                                   n_neighbours, neighbours );
                parameters[p] = (dtype) ((Real) save + step);
                f2 = evaluate_fit( n_parameters, parameters, volumes,
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

    FREE( volumes );
    FREE( parameters );
    FREE( xi );
    FREE( g );
    FREE( h );
    FREE( unit_dir );
}
