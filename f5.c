#include  <volume_io/internal_volume_io.h>
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
    int                  n_objects, n_i_objects, n_iters;
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
    dtype   heights[],
    int     n_neighbours[],
    int     *neighbours[] )
{
    int     p, n_points, max_neighbours, n, neigh, p_index, n_index;
    int     test;
    Real    fit;
    Point   *neigh_points;
    dtype   dist, len, radius, radius2;
    dtype   cx, cy, cz, nx, ny, nz, x2, y2, z2, factor;
    dtype   tx, ty, tz, x, y, z, height, dx, dy, dz;
    dtype   *lookup;

    fit = 0.0;

    radius = (dtype) global_radius;
    radius2 = radius * radius;

    n_points = (n_parameters-1) / 3;

    max_neighbours = 0;
    for_less( p, 0, n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[p] );

    ALLOC( neigh_points, max_neighbours );
    ALLOC( lookup, max_neighbours+1 );
    for_less( n, 1, max_neighbours+1 )
        lookup[n] = (dtype) (1.0 / (Real) n);

test = 10000;

    for_less( p, 0, n_points )
    {
        if( p == test )
            print( "here\n" );
        cx = 0.0f;
        cy = 0.0f;
        cz = 0.0f;
        nx = 0.0f;
        ny = 0.0f;
        nz = 0.0f;
        x2 = 0.0f;
        y2 = 0.0f;
        z2 = 0.0f;
        neigh = neighbours[p][n_neighbours[p]-1];
        n_index = IJ(neigh,0,3);
        tx = parameters[n_index+0];
        ty = parameters[n_index+1];
        tz = parameters[n_index+2];

        for_less( n, 0, n_neighbours[p] )
        {
            x = tx;
            y = ty;
            z = tz;

            neigh = neighbours[p][n];
            n_index = IJ(neigh,0,3);

            tx = parameters[n_index+0];
            ty = parameters[n_index+1];
            tz = parameters[n_index+2];
            cx += tx;
            cy += ty;
            cz += tz;
            x2 += tx * tx;
            y2 += ty * ty;
            z2 += tz * tz;

            nx += y * tz - ty * z;
            ny += z * tx - tz * x;
            nz += x * ty - tx * y;
        }

        factor = lookup[n_neighbours[p]];
        cx *= factor;
        cy *= factor;
        cz *= factor;

        len = sqrtf( nx * nx + ny * ny + nz * nz );
        dist = (x2 + y2 + z2) * factor - cx*cx - cy*cy - cz*cz;

        height = (radius - sqrtf(radius2-dist)) / len;

        p_index = IJ( p, 0, 3 );
        dx = parameters[p_index+0] - (cx + nx * height);
        dy = parameters[p_index+1] - (cy + ny * height);
        dz = parameters[p_index+2] - (cz + nz * height);
        fit += (Real) (dx * dx + dy * dy + dz * dz);
    }

    FREE( neigh_points );
    FREE( lookup );

    return( fit );
}

private  void  evaluate_fit_derivative(
    int      n_parameters,
    dtype    parameters[],
    dtype    heights[],
    int      n_neighbours[],
    int      *neighbours[],
    dtype    deriv[] )
{
    int    p, n_points, n, neigh, p_index, n_index, max_neighbours;
    Real   height, dist;
    Real   dx, dy, dz;
    Point  *neigh_points, this_point, centroid, model_point;
    Vector normal;

    for_less( p, 0, n_parameters )
        deriv[p] = (dtype) 0.0;

    n_points = (n_parameters-1) / 3;

    max_neighbours = 0;
    for_less( p, 0, n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[p] );

    ALLOC( neigh_points, max_neighbours );

    for_less( p, 0, n_points )
    {
        p_index = IJ(p,0,3);
        fill_Point( this_point,
                    parameters[p_index+0], parameters[p_index+1],
                    parameters[p_index+2] );
        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            n_index = IJ( neigh, 0, 3 );
            fill_Point( neigh_points[n],
                        parameters[n_index+0],
                        parameters[n_index+1],
                        parameters[n_index+2] );
        }

        get_points_centroid( n_neighbours[p], neigh_points, &centroid );
        find_polygon_normal( n_neighbours[p], neigh_points, &normal );

        dist = 0.0;
        for_less( n, 0, n_neighbours[p] )
            dist += sq_distance_between_points( &centroid, &neigh_points[n] );
        dist /= (Real) n_neighbours[p];

        height = global_radius -
                 sqrt( global_radius*global_radius - dist );
        GET_POINT_ON_RAY( model_point, centroid, normal, height );
           
        dx = RPoint_x(this_point) - RPoint_x(model_point);
        dy = RPoint_y(this_point) - RPoint_y(model_point);
        dz = RPoint_z(this_point) - RPoint_z(model_point);

        deriv[p_index+0] += (dtype) (2.0 * dx);
        deriv[p_index+1] += (dtype) (2.0 * dy);
        deriv[p_index+2] += (dtype) (2.0 * dz);

        dx /= (Real) n_neighbours[p];
        dy /= (Real) n_neighbours[p];
        dz /= (Real) n_neighbours[p];

        for_less( n, 0, n_neighbours[p] )
        {
            neigh = neighbours[p][n];
            n_index = IJ(neigh,0,3);
            deriv[n_index+0] += (dtype) (-2.0 * dx);
            deriv[n_index+1] += (dtype) (-2.0 * dy);
            deriv[n_index+2] += (dtype) (-2.0 * dz);
        }
    }

    FREE( neigh_points );
}

private  Real  evaluate_fit_along_line(
    int     n_parameters,
    dtype   parameters[],
    dtype   line_dir[],
    dtype   buffer[],
    Real    dist,
    dtype   heights[],
    int     n_neighbours[],
    int     *neighbours[] )
{
    int     p;
    Real    fit;
    
    for_less( p, 0, n_parameters )
        buffer[p] = (float) ( (Real) parameters[p] + dist * (Real) line_dir[p]);

    fit = evaluate_fit( n_parameters, buffer,
                        heights, n_neighbours, neighbours );

    return( fit );
}

#define  GOLDEN_RATIO   0.618034

private  Real  minimize_along_line(
    Real    current_fit,
    int     n_parameters,
    dtype   parameters[],
    dtype   line_dir[],
    dtype   heights[],
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
                                      heights, n_neighbours, neighbours );
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
                                      heights, n_neighbours, neighbours );
    }
    while( f2 <= f1 );

    n_iters = 0;
    do
    {
        t_next = t0 + (t1 - t0) * GOLDEN_RATIO;

        f_next = evaluate_fit_along_line( n_parameters, parameters, line_dir,
                                          test_parameters, t_next,
                                          heights, n_neighbours, neighbours );

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
    int              p, n, point, max_neighbours;
    int              n_parameters;
    Real             gg, dgg, gam, current_time, last_update_time;
    Real             fit, height;
    Real             len, radius, *x_sphere, *y_sphere, *z_sphere;
    Point            centroid, *neigh_points;
    int              iter, update_rate;
    dtype            *g, *h, *xi, *parameters, *unit_dir, *heights;
    BOOLEAN          init_supplied, changed;

    init_supplied = (init_points != NULL);
    if( !init_supplied )
        init_points = points;

    radius = 0.0;
    get_points_centroid( n_points, init_points, &centroid );
    for_less( point, 0, n_points )
        radius += distance_between_points( &points[point], &centroid );
    radius /= (Real) n_points;

    global_radius = radius;

    ALLOC( heights, n_points );

    max_neighbours = 0;
    for_less( p, 0, n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[p] );

    ALLOC( neigh_points, max_neighbours );
    ALLOC( x_sphere, max_neighbours );
    ALLOC( y_sphere, max_neighbours );
    ALLOC( z_sphere, max_neighbours );

    for_less( point, 0, n_points )
    {
        for_less( n, 0, n_neighbours[point] )
            neigh_points[n] = points[neighbours[point][n]];

        flatten_around_vertex_to_sphere( radius, &points[point],
                                         n_neighbours[point], neigh_points,
                                         x_sphere, y_sphere, z_sphere );


        height = 0.0;
        for_less( n, 0, n_neighbours[point] )
            height += -z_sphere[n];
        height /= (Real) n_neighbours[point];

        heights[point] = (dtype) height;
    }

    FREE( neigh_points );
    FREE( x_sphere );
    FREE( y_sphere );
    FREE( z_sphere );

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

    fit = evaluate_fit( n_parameters, parameters, heights,
                        n_neighbours, neighbours );

    print( "Initial  %g\n", fit );
    (void) flush_file( stdout );

    evaluate_fit_derivative( n_parameters, parameters, heights,
                             n_neighbours, neighbours, xi );

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

        fit = minimize_along_line( fit, n_parameters, parameters, unit_dir,
                                   heights, n_neighbours, neighbours,
                                   &changed );

        if( !changed )
            break;

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

        evaluate_fit_derivative( n_parameters, parameters, heights,
                                 n_neighbours, neighbours, xi );

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

    FREE( heights );
    FREE( parameters );
    FREE( xi );
    FREE( g );
    FREE( h );
    FREE( unit_dir );
}
