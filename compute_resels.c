#include  <volume_io.h>
#include  <bicpl.h>

#undef   DEBUG
#define  DEBUG

static  VIO_Real  compute_resels(
    polygons_struct   *average_polygons,
    int               n_surfaces[2],
    VIO_Point             **samples[2],
    VIO_Real              *fwhm );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR           filename;
    int              i, n_objects, n_surfaces[2], group, n_points, s;
    VIO_File_formats     format;
    object_struct    **object_list;
    polygons_struct  polygons;
    VIO_Point            *points, *copy_points, **samples[2];
    VIO_Real             resels, fwhm;

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
                                 &object_list ) != VIO_OK ||
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

    print( "N samples: %d %d\n", n_surfaces[0], n_surfaces[1] );

    if( n_surfaces[0] + n_surfaces[1] == 0 )
    {
        print_error( "Usage: %s [surfA1.obj] [surfA2.obj] ... + [surfB1.obj] [surfB2.obj]... \n",
                     argv[0] );
        return( 1 );
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
                     1.0 / (VIO_Real) (n_surfaces[0] + n_surfaces[1]) );

    resels = compute_resels( &polygons, n_surfaces, samples, &fwhm );

    print( "Resels: %g\n", resels );
    print( "FWHM:   %g\n", fwhm );

    return( 0 );
}

#define   SQRT_TOLERANCE   1.0e-10

static  VIO_Real  safe_sqrt(
    VIO_Real   s )
{
    if( s < -SQRT_TOLERANCE )
        print_error( "safe_sqrt: %g\n", s );
    else if( s <= 0.0 )
        return( 0.0 );

    return( sqrt(s) );
}

static  void  compute_upper_triangular_Cholesky(
    VIO_Real    s[3][3],
    VIO_Real    **sqrt_s )
{
    sqrt_s[0][0] = safe_sqrt( s[0][0] );
    sqrt_s[0][1] = s[0][1] / sqrt_s[0][0];
    sqrt_s[0][2] = s[0][2] / sqrt_s[0][0];
    sqrt_s[1][1] = safe_sqrt( s[1][1] - sqrt_s[0][1] * sqrt_s[0][1] );
    sqrt_s[1][2] = (s[1][2] - sqrt_s[0][1] * sqrt_s[0][2]) / sqrt_s[1][1];
    sqrt_s[2][2] = safe_sqrt( s[2][2] - sqrt_s[0][2] * sqrt_s[0][2] -
                              sqrt_s[1][2] * sqrt_s[1][2] );

    sqrt_s[1][0] = 0.0;
    sqrt_s[2][0] = 0.0;
    sqrt_s[2][1] = 0.0;
}

#define  TOLERANCE 1.0e-4

static  void  check_Cholesky(
    VIO_Real    s[3][3],
    VIO_Real    **sqrt_s )
{
    int   i, j, k;
    VIO_Real  prod;

    for_less( i, 0, VIO_N_DIMENSIONS )
    for_less( j, 0, VIO_N_DIMENSIONS )
    {
        prod = 0.0;
        for_less( k, 0, VIO_N_DIMENSIONS )
        {
            prod += sqrt_s[k][i] * sqrt_s[k][j];
        }

        if( !numerically_close( prod, s[i][j], TOLERANCE ) )
        {
            print_error( "check_Cholesky: %g %g\n", prod, s[i][j] );
        }
    }
}

static  void  check_UV(
    int     n_surfaces[2],
    VIO_Real    **U,
    VIO_Real    **V )
{
    int   i, j, s, v;
    VIO_Real  prod, desired;

    v = n_surfaces[0] + n_surfaces[1] - 2;

    for_less( i, 0, VIO_N_DIMENSIONS )
    for_less( j, 0, VIO_N_DIMENSIONS )
    {
        prod = 0.0;
        for_less( s, 0, n_surfaces[0] )
            prod += U[s][i] * U[s][j];
        for_less( s, 0, n_surfaces[1] )
            prod += V[s][i] * V[s][j];

        prod /= (VIO_Real) v;

        desired = (VIO_Real) (i == j);

        if( prod >= desired + TOLERANCE || prod <= desired - TOLERANCE )
        {
            print_error( "Check UV: %d %d %g\n", i, j, prod );
        }
    }
}

static  void  compute_UV(
    int     n_surfaces[2],
    VIO_Point   **samples[2],
    int     point_index,
    VIO_Real    **U,
    VIO_Real    **V )
{
    int     c, s, i, j, v, dim;
    VIO_Real    mean1[3], mean2[3], variance[3][3];
    VIO_Real    **sqrt_s, **inverse_sqrt_s;
    VIO_Real    dx, dy, dz;

    v = n_surfaces[0] + n_surfaces[1] - 2;

    for_less( c, 0, 3 )
    {
        mean1[c] = 0.0;
        mean2[c] = 0.0;
    }

    for_less( s, 0, n_surfaces[0] )
    {
        for_less( c, 0, 3 )
            mean1[c] += (VIO_Real) Point_coord(samples[0][s][point_index],c);
    }

    for_less( s, 0, n_surfaces[1] )
    {
        for_less( c, 0, 3 )
            mean2[c] += (VIO_Real) Point_coord(samples[1][s][point_index],c);
    }

    for_less( c, 0, 3 )
    {
        mean1[c] /= (VIO_Real) n_surfaces[0];
        mean2[c] /= (VIO_Real) n_surfaces[1];
    }

    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
    {
        variance[i][j] = 0.0;
    }

    for_less( s, 0, n_surfaces[0] )
    {
        dx = (VIO_Real) Point_x(samples[0][s][point_index]) - mean1[VIO_X];
        dy = (VIO_Real) Point_y(samples[0][s][point_index]) - mean1[VIO_Y];
        dz = (VIO_Real) Point_z(samples[0][s][point_index]) - mean1[VIO_Z];

        variance[0][0] += dx * dx;
        variance[0][1] += dx * dy;
        variance[0][2] += dx * dz;
        variance[1][1] += dy * dy;
        variance[1][2] += dy * dz;
        variance[2][2] += dz * dz;
    }

    for_less( s, 0, n_surfaces[1] )
    {
        dx = (VIO_Real) Point_x(samples[1][s][point_index]) - mean2[VIO_X];
        dy = (VIO_Real) Point_y(samples[1][s][point_index]) - mean2[VIO_Y];
        dz = (VIO_Real) Point_z(samples[1][s][point_index]) - mean2[VIO_Z];

        variance[0][0] += dx * dx;
        variance[0][1] += dx * dy;
        variance[0][2] += dx * dz;
        variance[1][1] += dy * dy;
        variance[1][2] += dy * dz;
        variance[2][2] += dz * dz;
    }

    variance[1][0] = variance[0][1];
    variance[2][0] = variance[0][2];
    variance[2][1] = variance[1][2];

    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
    {
        variance[i][j] /= (VIO_Real) v;
    }

    VIO_ALLOC2D( sqrt_s, 3, 3 );
    VIO_ALLOC2D( inverse_sqrt_s, 3, 3 );

    compute_upper_triangular_Cholesky( variance, sqrt_s );

    check_Cholesky( variance, sqrt_s );

    if( !invert_square_matrix( 3, sqrt_s, inverse_sqrt_s ) )
    {
        print_error( "Error getting inverse of sqrt of variance\n" );
    }

    for_less( s, 0, n_surfaces[0] )
    {
        dx = (VIO_Real) Point_x(samples[0][s][point_index]) - mean1[VIO_X];
        dy = (VIO_Real) Point_y(samples[0][s][point_index]) - mean1[VIO_Y];
        dz = (VIO_Real) Point_z(samples[0][s][point_index]) - mean1[VIO_Z];

        for_less( dim, 0, VIO_N_DIMENSIONS )
        {
            U[s][dim] = inverse_sqrt_s[0][dim] * dx +
                        inverse_sqrt_s[1][dim] * dy +
                        inverse_sqrt_s[2][dim] * dz;
        }
    }

    for_less( s, 0, n_surfaces[1] )
    {
        dx = (VIO_Real) Point_x(samples[1][s][point_index]) - mean2[VIO_X];
        dy = (VIO_Real) Point_y(samples[1][s][point_index]) - mean2[VIO_Y];
        dz = (VIO_Real) Point_z(samples[1][s][point_index]) - mean2[VIO_Z];

        for_less( dim, 0, VIO_N_DIMENSIONS )
        {
            V[s][dim] = inverse_sqrt_s[0][dim] * dx +
                        inverse_sqrt_s[1][dim] * dy +
                        inverse_sqrt_s[2][dim] * dz;
        }
    }

    check_UV( n_surfaces, U, V );

    VIO_FREE2D( sqrt_s );
    VIO_FREE2D( inverse_sqrt_s );
}

static  VIO_Real  compute_lambda(
    int      n_surfaces[2],
    int      p1,
    int      p2,
    VIO_Real     **U_p1,
    VIO_Real     **U_p2,
    VIO_Real     **V_p1,
    VIO_Real     **V_p2,
    VIO_Point    mean_points[] )
{
    int   v, s, n, dim;
    VIO_Real  lambda, dist_between, **delta_u, **delta_v;

    v = n_surfaces[0] + n_surfaces[1] - 2;
    n = v - VIO_N_DIMENSIONS + 1;

    dist_between = distance_between_points( &mean_points[p1], &mean_points[p2]);

    VIO_ALLOC2D( delta_u, n_surfaces[0], VIO_N_DIMENSIONS );
    VIO_ALLOC2D( delta_v, n_surfaces[1], VIO_N_DIMENSIONS );

    for_less( s, 0, n_surfaces[0] )
    {
        for_less( dim, 0, VIO_N_DIMENSIONS )
            delta_u[s][dim] = (U_p1[s][dim] - U_p2[s][dim]) / dist_between;
    }

    for_less( s, 0, n_surfaces[1] )
    {
        for_less( dim, 0, VIO_N_DIMENSIONS )
            delta_v[s][dim] = (V_p1[s][dim] - V_p2[s][dim]) / dist_between;
    }

    lambda = 0.0;
    for_less( s, 0, n_surfaces[0] )
    {
        for_less( dim, 0, VIO_N_DIMENSIONS )
            lambda += delta_u[s][dim] * delta_u[s][dim];
    }
    for_less( s, 0, n_surfaces[1] )
    {
        for_less( dim, 0, VIO_N_DIMENSIONS )
            lambda += delta_v[s][dim] * delta_v[s][dim];
    }

/*
    lambda *= (VIO_Real) (n - 2) / (VIO_Real) (n - 1) / (VIO_Real) VIO_N_DIMENSIONS /
              (VIO_Real) n;   why the division by n?
*/
    lambda *= (VIO_Real) (n - 2) / (VIO_Real) (n - 1) / (VIO_Real) VIO_N_DIMENSIONS;

    VIO_FREE2D( delta_u );
    VIO_FREE2D( delta_v );

    return( lambda );
}

static  VIO_Real  compute_resels(
    polygons_struct   *average_polygons,
    int               n_surfaces[2],
    VIO_Point             **samples[2],
    VIO_Real              *fwhm )
{
    int    poly, size, edge, p1, p2, n_edges, point_index;
    VIO_Real   sum_lambda, lambda, area, resels;
    VIO_Real   ***U, ***V;

    VIO_ALLOC3D( U, average_polygons->n_points, n_surfaces[0], VIO_N_DIMENSIONS );
    VIO_ALLOC3D( V, average_polygons->n_points, n_surfaces[1], VIO_N_DIMENSIONS );

    for_less( point_index, 0, average_polygons->n_points )
    {
        compute_UV( n_surfaces, samples, point_index,
                    U[point_index], V[point_index] );
    }

    sum_lambda = 0.0;
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

            sum_lambda += compute_lambda( n_surfaces, p1, p2,
                                          U[p1], U[p2], V[p1], V[p2],
                                          average_polygons->points );
#ifdef DEBUG
if( n_edges == 0 )
{
    int   s;
    VIO_Real  dist_between;

    dist_between = distance_between_points( &average_polygons->points[p1],
                                            &average_polygons->points[p2] );

    print( "Lambda for vertex %d minus %d: %g\n", p1, p2, sum_lambda );

    print( "Distance between average vertex %d and %d:  %g\n", p1, p2,
           dist_between );

    print( "\nX[%d]:\n\n", p1 );
    for_less( s, 0, n_surfaces[0] )
        print( "%g %g %g\n", Point_x(samples[0][s][p1]),
                             Point_y(samples[0][s][p1]),
                             Point_z(samples[0][s][p1]) );

    print( "\nX[%d]:\n\n", p2 );
    for_less( s, 0, n_surfaces[0] )
        print( "%g %g %g\n", Point_x(samples[0][s][p2]),
                             Point_y(samples[0][s][p2]),
                             Point_z(samples[0][s][p2]) );

    print( "\nY[%d]:\n\n", p1 );
    for_less( s, 0, n_surfaces[1] )
        print( "%g %g %g\n", Point_x(samples[1][s][p1]),
                             Point_y(samples[1][s][p1]),
                             Point_z(samples[1][s][p1]) );

    print( "\nY[%d]:\n\n", p2 );
    for_less( s, 0, n_surfaces[1] )
        print( "%g %g %g\n", Point_x(samples[1][s][p2]),
                             Point_y(samples[1][s][p2]),
                             Point_z(samples[1][s][p2]) );
}
#endif

            ++n_edges;
        }
    }

    lambda = sum_lambda / (VIO_Real) n_edges;

    *fwhm = sqrt( 4.0 * log( 2.0 ) / lambda );

    area = get_polygons_surface_area( average_polygons );

    print( "Surface area: %g\n", area );

    resels = area / *fwhm / *fwhm;

    return( resels );
}    
