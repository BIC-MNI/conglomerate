#include  <internal_volume_io.h>
#include  <bicpl.h>

private  Real  compute_resels(
    polygons_struct   *average_polygons,
    int               n_surfaces[2],
    Point             **samples[2],
    Real              *fwhm );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           filename;
    int              i, n_objects, n_surfaces[2], group, n_points, s;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  polygons;
    Point            *points, *copy_points, **samples[2];
    Real             resels, fwhm;

    initialize_argument_processing( argc, argv );

    n_surfaces[0] = 0;
    n_surfaces[1] = 0;
    group = 0;

    while( get_string_argument( NULL, &filename ) )
    {
        if( equal_strings( filename, "+" ) )
        {
            ++group;
            if( group > 1 )
            {
                print_error( "Too many +\n" );
                return( 1 );
            }
            continue;
        }

        if( input_graphics_file( filename, &format, &n_objects,
                                 &object_list ) != OK ||
            n_objects != 1 ||
            get_object_type(object_list[0]) != POLYGONS )
        {
            print( "Error reading %s.\n", filename );
            return( 1 );
        }

        if( n_surfaces[0] == 0 )
        {
            copy_polygons( get_polygons_ptr(object_list[0]), &polygons );
            n_points = polygons.n_points;
        }

        if( get_object_points( object_list[0], &points ) != n_points )
        {
            print_error( "Invalid number of points in %s\n", filename );
            return( 1 );
        }

        ALLOC( copy_points, n_points );
        for_less( i, 0, n_points )
            copy_points[i] = points[i];

        ADD_ELEMENT_TO_ARRAY( samples[group], n_surfaces[group],
                              copy_points, 1 );

        delete_object_list( n_objects, object_list );
    }

    for_less( i, 0, n_points )
        fill_Point( polygons.points[i], 0.0, 0.0, 0.0 );

    for_less( s, 0, n_surfaces[0] )
    {
        for_less( i, 0, n_points )
            ADD_POINTS( polygons.points[i], polygons.points[i],
                        samples[0][s][i] );
    }

    for_less( s, 0, n_surfaces[1] )
    {
        for_less( i, 0, n_points )
            ADD_POINTS( polygons.points[i], polygons.points[i],
                        samples[1][s][i] );
    }

    for_less( i, 0, n_points )
        SCALE_POINT( polygons.points[i], polygons.points[i],
                     1.0 / (Real) (n_surfaces[0] + n_surfaces[1]) );

    resels = compute_resels( &polygons, n_surfaces, samples, &fwhm );

    print( "Resels: %g\n", resels );
    print( "FWHM:   %g\n", fwhm );

    return( 0 );
}

private  Real  compute_Q(
    int     n_surfaces[2],
    Point   **samples[2],
    Point   average_points[],
    int     p1,
    int     p2 )
{
    int     c, s, i, j;
    Real    mean1[3], mean2[3], **variance1, **variance2, **variance, **inverse;
    Real    mean_node1[2][3], mean_node2[2][3];
    Real    dx, dy, dz, q, tx, ty, tz, len, **delta1, **delta2;
    Real    avg_dx, avg_dy, avg_dz;

    ALLOC2D( delta1, n_surfaces[0], 3 );
    ALLOC2D( delta2, n_surfaces[1], 3 );

    for_less( c, 0, 3 )
    {
        mean1[c] = 0.0;
        mean2[c] = 0.0;
    }

    for_less( s, 0, n_surfaces[0] )
    {
        dx = Point_x(samples[0][s][p1]) - Point_x(samples[0][s][p2]);
        dy = Point_y(samples[0][s][p1]) - Point_y(samples[0][s][p2]);
        dz = Point_z(samples[0][s][p1]) - Point_z(samples[0][s][p2]);

        avg_dx = Point_x(average_points[p1]) - Point_x(average_points[p2]);
        avg_dy = Point_y(average_points[p1]) - Point_y(average_points[p2]);
        avg_dz = Point_z(average_points[p1]) - Point_z(average_points[p2]);

        len = sqrt( avg_dx * avg_dx + avg_dy * avg_dy + avg_dz * avg_dz );

        delta1[s][X] = dx / len;
        delta1[s][Y] = dy / len;
        delta1[s][Z] = dz / len;

        for_less( c, 0, 3 )
            mean1[c] += delta1[s][c];
    }

    for_less( s, 0, n_surfaces[1] )
    {
        dx = Point_x(samples[1][s][p1]) - Point_x(samples[1][s][p2]);
        dy = Point_y(samples[1][s][p1]) - Point_y(samples[1][s][p2]);
        dz = Point_z(samples[1][s][p1]) - Point_z(samples[1][s][p2]);

        avg_dx = Point_x(average_points[p1]) - Point_x(average_points[p2]);
        avg_dy = Point_y(average_points[p1]) - Point_y(average_points[p2]);
        avg_dz = Point_z(average_points[p1]) - Point_z(average_points[p2]);

        len = sqrt( avg_dx * avg_dx + avg_dy * avg_dy + avg_dz * avg_dz );

        delta2[s][X] = dx / len;
        delta2[s][Y] = dy / len;
        delta2[s][Z] = dz / len;

        for_less( c, 0, 3 )
            mean2[c] += delta2[s][c];
    }

    for_less( c, 0, 3 )
    {
        mean1[c] /= (Real) n_surfaces[0];
        mean2[c] /= (Real) n_surfaces[1];
    }

    /*--- compute means at each node */

    for_less( c, 0, 3 )
    {
        mean_node1[0][c] = 0.0;
        mean_node1[1][c] = 0.0;
        mean_node2[0][c] = 0.0;
        mean_node2[1][c] = 0.0;
    }

    for_less( s, 0, n_surfaces[0] )
    {
        for_less( c, 0, 3 )
        {
            mean_node1[0][c] += Point_coord(samples[0][s][p1],c);
            mean_node2[0][c] += Point_coord(samples[0][s][p2],c);
        }
    }

    for_less( s, 0, n_surfaces[1] )
    {
        for_less( c, 0, 3 )
        {
            mean_node1[1][c] += Point_coord(samples[1][s][p1],c);
            mean_node2[1][c] += Point_coord(samples[1][s][p2],c);
        }
    }

    for_less( c, 0, 3 )
    {
        mean_node1[0][c] /= (Real) n_surfaces[0];
        mean_node1[1][c] /= (Real) n_surfaces[1];
        mean_node2[0][c] /= (Real) n_surfaces[0];
        mean_node2[1][c] /= (Real) n_surfaces[1];
    }

    /*--- compute the variance at each of the two nodes */

    ALLOC2D( variance1, 3, 3 );
    ALLOC2D( variance2, 3, 3 );

    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
    {
        variance1[i][j] = 0.0;
        variance2[i][j] = 0.0;
    }

    for_less( s, 0, n_surfaces[0] )
    {
        dx = Point_x(samples[0][s][p1]) - mean_node1[0][0];
        dy = Point_y(samples[0][s][p1]) - mean_node1[0][1];
        dz = Point_z(samples[0][s][p1]) - mean_node1[0][2];

        variance1[0][0] += dx * dx;
        variance1[0][1] += dx * dy;
        variance1[0][2] += dx * dz;
        variance1[1][1] += dy * dy;
        variance1[1][2] += dy * dz;
        variance1[2][2] += dz * dz;

        dx = Point_x(samples[0][s][p2]) - mean_node2[0][0];
        dy = Point_y(samples[0][s][p2]) - mean_node2[0][1];
        dz = Point_z(samples[0][s][p2]) - mean_node2[0][2];

        variance2[0][0] += dx * dx;
        variance2[0][1] += dx * dy;
        variance2[0][2] += dx * dz;
        variance2[1][1] += dy * dy;
        variance2[1][2] += dy * dz;
        variance2[2][2] += dz * dz;
    }

    for_less( s, 0, n_surfaces[1] )
    {
        dx = Point_x(samples[1][s][p1]) - mean_node1[1][0];
        dy = Point_y(samples[1][s][p1]) - mean_node1[1][1];
        dz = Point_z(samples[1][s][p1]) - mean_node1[1][2];

        variance1[0][0] += dx * dx;
        variance1[0][1] += dx * dy;
        variance1[0][2] += dx * dz;
        variance1[1][1] += dy * dy;
        variance1[1][2] += dy * dz;
        variance1[2][2] += dz * dz;

        dx = Point_x(samples[1][s][p2]) - mean_node2[1][0];
        dy = Point_y(samples[1][s][p2]) - mean_node2[1][1];
        dz = Point_z(samples[1][s][p2]) - mean_node2[1][2];

        variance2[0][0] += dx * dx;
        variance2[0][1] += dx * dy;
        variance2[0][2] += dx * dz;
        variance2[1][1] += dy * dy;
        variance2[1][2] += dy * dz;
        variance2[2][2] += dz * dz;
    }

    variance1[1][0] = variance1[0][1];
    variance1[2][0] = variance1[0][2];
    variance1[2][1] = variance1[1][2];
    variance2[1][0] = variance2[0][1];
    variance2[2][0] = variance2[0][2];
    variance2[2][1] = variance2[1][2];

    ALLOC2D( variance, 3, 3 );
    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
    {
        variance1[i][j] /= (Real) (n_surfaces[0] + n_surfaces[1] - 2);
        variance2[i][j] /= (Real) (n_surfaces[0] + n_surfaces[1] - 2);
        variance[i][j] = (variance1[i][j] + variance2[i][j]) / 2.0;
    }

    ALLOC2D( inverse, 3, 3 );

    if( !invert_square_matrix( 3, variance, inverse ) )
        print_error( "Error getting inverse of variance\n" );

    q = 0.0;

    for_less( s, 0, n_surfaces[0] )
    {
        dx = delta1[s][0] - mean1[0];
        dy = delta1[s][1] - mean1[1];
        dz = delta1[s][2] - mean1[2];

        tx = dx * inverse[0][0] + dy * inverse[1][0] + dz * inverse[2][0];
        ty = dx * inverse[0][1] + dy * inverse[1][1] + dz * inverse[2][1];
        tz = dx * inverse[0][2] + dy * inverse[1][2] + dz * inverse[2][2];

        q += tx * dx + ty * dy + tz * dz;
    }

    for_less( s, 0, n_surfaces[1] )
    {
        dx = delta2[s][0] - mean2[0];
        dy = delta2[s][1] - mean2[1];
        dz = delta2[s][2] - mean2[2];

        tx = dx * inverse[0][0] + dy * inverse[1][0] + dz * inverse[2][0];
        ty = dx * inverse[0][1] + dy * inverse[1][1] + dz * inverse[2][1];
        tz = dx * inverse[0][2] + dy * inverse[1][2] + dz * inverse[2][2];

        q += tx * dx + ty * dy + tz * dz;
    }

    FREE2D( variance1 );
    FREE2D( variance2 );
    FREE2D( variance );
    FREE2D( inverse );
    FREE2D( delta1 );
    FREE2D( delta2 );

    return( q );
}

private  Real  compute_resels(
    polygons_struct   *average_polygons,
    int               n_surfaces[2],
    Point             **samples[2],
    Real              *fwhm )
{
    int    D, m, n, v, poly, size, edge, p1, p2, n_edges;
    Real   sum_Q, lambda, area, resels, avg_dist;

    sum_Q = 0.0;
    avg_dist = 0.0;
    n_edges = 0;

    for_less( poly, 0, average_polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *average_polygons, poly );

        for_less( edge, 0, size )
        {
            p1 = average_polygons->indices[
                    POINT_INDEX(average_polygons->end_indices,poly,edge)];
            p2 = average_polygons->indices[
                    POINT_INDEX(average_polygons->end_indices,poly,
                                              (edge+1)%size)];

            avg_dist += distance_between_points( &average_polygons->points[p1],
                                                 &average_polygons->points[p2]);

            sum_Q += compute_Q( n_surfaces, samples, average_polygons->points,
                                p1, p2 );
            ++n_edges;
        }
    }

    avg_dist /= (Real) n_edges;

    print( "Avg dist: %g\n", avg_dist );

    D = 3;
    m = n_surfaces[0];
    n = n_surfaces[1];
    v = m + n - D - 1;

    lambda = (Real) (v - 2) / (Real) (D * (n + m - 2) * (n + m - 2)) *
             sum_Q / n_edges;

    *fwhm = sqrt( 4.0 * log( 2.0 ) / lambda );

    area = get_polygons_surface_area( average_polygons );

    print( "Surface area: %g\n", area );

    resels = area / *fwhm / *fwhm;

    return( resels );
}    
