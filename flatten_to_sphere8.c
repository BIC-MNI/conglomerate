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
        print_error( "Usage: %s  input.obj output.obj [n_iters] [init]\n",
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
    int     p, n_points, n, p_index, prev_index, ind;
    int     n_index, next_index;
    Real    fit, diff;
    dtype   x1, y1, z1, x2, y2, z2, vol;
    dtype   x3, y3, z3, cx, cy, cz, x4, y4, z4;
    dtype   dx2, dy2, dz2, dx3, dy3, dz3, dx4, dy4, dz4;

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
            prev_index = IJ(neighbours[p][(n-1+n_neighbours[p])%
                                        n_neighbours[p]],0,3);
            x2 = parameters[prev_index+0];
            y2 = parameters[prev_index+1];
            z2 = parameters[prev_index+2];

            n_index = IJ(neighbours[p][n],0,3);
            x3 = parameters[n_index+0];
            y3 = parameters[n_index+1];
            z3 = parameters[n_index+2];

            next_index = IJ(neighbours[p][(n+1)%n_neighbours[p]],0,3);
            x4 = parameters[next_index+0];
            y4 = parameters[next_index+1];
            z4 = parameters[next_index+2];

            dx2 = x2 - x1;
            dy2 = y2 - y1;
            dz2 = z2 - z1;

            dx3 = x3 - x1;
            dy3 = y3 - y1;
            dz3 = z3 - z1;

            dx4 = x4 - x1;
            dy4 = y4 - y1;
            dz4 = z4 - z1;

            cx = dy2 * dz3 - dz2 * dy3;
            cy = dz2 * dx3 - dx2 * dz3;
            cz = dx2 * dy3 - dy2 * dx3;

            vol = cx * dx4 + cy * dy4 + cz * dz4;
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
    int     p, n_points, n, p_index, prev_index, ind;
    int     n_index, next_index;
    dtype   x1, y1, z1, x2, y2, z2, vol, d2, diff;
    dtype   x3, y3, z3, cx, cy, cz, x4, y4, z4;
    dtype   dx2, dy2, dz2, dx3, dy3, dz3, dx4, dy4, dz4;

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
            prev_index = IJ(neighbours[p][(n-1+n_neighbours[p])%
                                        n_neighbours[p]],0,3);
            x2 = parameters[prev_index+0];
            y2 = parameters[prev_index+1];
            z2 = parameters[prev_index+2];

            n_index = IJ(neighbours[p][n],0,3);
            x3 = parameters[n_index+0];
            y3 = parameters[n_index+1];
            z3 = parameters[n_index+2];

            next_index = IJ(neighbours[p][(n+1)%n_neighbours[p]],0,3);
            x4 = parameters[next_index+0];
            y4 = parameters[next_index+1];
            z4 = parameters[next_index+2];

            dx2 = x2 - x1;
            dy2 = y2 - y1;
            dz2 = z2 - z1;

            dx3 = x3 - x1;
            dy3 = y3 - y1;
            dz3 = z3 - z1;

            dx4 = x4 - x1;
            dy4 = y4 - y1;
            dz4 = z4 - z1;

            cx = dy2 * dz3 - dz2 * dy3;
            cy = dz2 * dx3 - dx2 * dz3;
            cz = dx2 * dy3 - dy2 * dx3;

            vol = cx * dx4 + cy * dy4 + cz * dz4;
            diff = vol - volumes[ind];
            ++ind;
            d2 = 2.0f * diff;

            deriv[p_index+0] += d2 * ((y1-y2)*(z3-z1) + (z2-z1)*(y3-y1) +
                                      (z3-z2)*(y4-y1) + (y2-y3)*(z4-z1));
            deriv[p_index+1] += d2 * ((z1-z2)*(x3-x1) + (x2-x1)*(z3-z1) +
                                      (x3-x2)*(z4-z1) + (z2-z3)*(x4-x1));
            deriv[p_index+2] += d2 * ((x1-x2)*(y3-y1) + (y2-y1)*(x3-x1) +
                                      (y3-y2)*(x4-x1) + (x2-x3)*(y4-y1));

            deriv[prev_index+0] += d2 * ((z1-z3)*(y4-y1) + (y3-y1)*(z4-z1));
            deriv[prev_index+1] += d2 * ((x1-x3)*(z4-z1) + (z3-z1)*(x4-x1));
            deriv[prev_index+2] += d2 * ((y1-y3)*(x4-x1) + (x3-x1)*(y4-y1));

            deriv[n_index+0] += d2 * ((z2-z1)*(y4-y1) + (y1-y2)*(z4-z1));
            deriv[n_index+1] += d2 * ((x2-x1)*(z4-z1) + (z1-z2)*(x4-x1));
            deriv[n_index+2] += d2 * ((y2-y1)*(x4-x1) + (x1-x2)*(y4-y1));

            deriv[next_index+0] += d2 * ((y2-y1)*(z3-z1) - (z2-z1)*(y3-y1));
            deriv[next_index+1] += d2 * ((z2-z1)*(x3-x1) - (x2-x1)*(z3-z1));
            deriv[next_index+2] += d2 * ((x2-x1)*(y3-y1) - (y2-y1)*(x3-x1));
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

private  void  get_third_point(
    Real      radius,
    Point     *p1,
    Point     *p2,
    Point     *p3,
    Point     *s1,
    Point     *s2,
    Point     *s3 )
{
    Real    d2, a, u, h, fraction;
    Vector  p12, p13, up, hor, s12;
    Point   m;

    SUB_POINTS( p12, *p2, *p1 );
    SUB_POINTS( p13, *p3, *p1 );

    fraction = DOT_VECTORS( p12, p13 ) / DOT_VECTORS( p12, p12 );

    INTERPOLATE_POINTS( m, *p1, *p2, fraction );
    d2 = distance_between_points( &m, p3 );
    a = radius - sqrt( DOT_POINTS( m, m ) );

    INTERPOLATE_POINTS( m, *s1, *s2, fraction );

    CONVERT_POINT_TO_VECTOR( up, m );
    SUB_POINTS( s12, *s2, *s1 );
    CROSS_VECTORS( hor, up, s12 );

    NORMALIZE_VECTOR( up, up );
    NORMALIZE_VECTOR( hor, hor );

    u = (2.0 * radius * a - a * a - d2*d2) / (2.0 * a - 2.0 * radius);
    h = sqrt( d2*d2 - u * u);

    SCALE_VECTOR( up, up, u );
    SCALE_VECTOR( hor, hor, h );

    ADD_POINT_VECTOR( *s3, m, up );
    ADD_POINT_VECTOR( *s3, *s3, hor );

    if( !numerically_close( DOT_POINTS(*s1,*s1), radius*radius, 1.0e-4 ) )
        handle_internal_error( "Nope s1" );
    if( !numerically_close( DOT_POINTS(*s2,*s2), radius*radius, 1.0e-4 ) )
        handle_internal_error( "Nope s2" );
    if( !numerically_close( DOT_POINTS(*s3,*s3), radius*radius, 1.0e-4 ) )
        handle_internal_error( "Nope s3" );
    if( !numerically_close( distance_between_points(p1,p3),
                            distance_between_points(s1,s3), 1.0e-4 ) )
        handle_internal_error( "No way 1" );

    if( !numerically_close( distance_between_points(p2,p3),
                            distance_between_points(s2,s3), 1.0e-4 ) )
        handle_internal_error( "No wayi 2" );
}

private  void  compute_sphere_points(
    Real       radius,
    Point      *p1,
    Point      *p2,
    Point      *p3,
    Point      *p4,
    Vector     *v2,
    Vector     *v3,
    Vector     *v4 )
{
    Real    x, y, d12;
    Point   s1, s2, s3, s4;

    fill_Point( s1, radius, 0.0, 0.0 );

    d12 = distance_between_points( p1, p2 );
    x = (2.0 * radius * radius - d12 * d12) / (2.0 * radius);
    y = sqrt( radius * radius - x * x );

    fill_Point( s2, x, y, 0.0 );

    get_third_point( radius, p1, p2, p3, &s1, &s2, &s3 );
    get_third_point( radius, p1, p3, p4, &s1, &s3, &s4 );

    SUB_POINTS( *v2, s2, s1 );
    SUB_POINTS( *v3, s3, s1 );
    SUB_POINTS( *v4, s4, s1 );
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
    int              p, point, n_parameters, ind;
    Real             gg, dgg, gam, current_time, last_update_time, fit;
    Real             len, step, d;
    Point            p1, p2, p3, p4;
    Vector           v2, v3, v4, cross;
    int              iter, update_rate, n, total_neighbours;
    dtype            *g, *h, *xi, *parameters, *unit_dir, *volumes;
    BOOLEAN          init_supplied, changed, debug, testing;

    debug = getenv( "DEBUG" ) != NULL;

    init_supplied = (init_points != NULL);
    if( !init_supplied )
        init_points = points;

    total_neighbours = 0;
    for_less( p, 0, n_points )
        total_neighbours += n_neighbours[p];

    ALLOC( volumes, total_neighbours );

    ind = 0;
    for_less( point, 0, n_points )
    {
        p1 = points[point];

        for_less( n, 0, n_neighbours[point] )
        {
            p2 = points[neighbours[point][
                               (n-1+n_neighbours[point])%n_neighbours[point]]];
            p3 = points[neighbours[point][n]];
            p4 = points[neighbours[point][(n+1)%n_neighbours[point]]];

            compute_sphere_points( radius, &p1, &p2, &p3, &p4,
                                   &v2, &v3, &v4 );

            CROSS_VECTORS( cross, v2, v3 );
            d = DOT_VECTORS( cross, v4 );

            volumes[ind] = (float) d;

            ++ind;
        }
    }

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
