#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void   compute_inv_variance(
    int     n_surfaces[2],
    Point   **samples[2],
    int     p,
    Point   *avg1,
    Point   *avg2,
    Real    **inv_s );

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           filename, output_filename;
    int              i, n_objects, n_surfaces[2], group, n_points, s, p;
    int              v, nu, which;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  polygons;
    Point            *points, *copy_points, **samples[2];
    Point            *avg1_points, *avg2_points;
    Vector           offset;
    Real             **inv_s, tx, ty, tz, mahalanobis, variance;
    Real             mean[2], var[2], sum_x, sum_xx, dist;
    Real             conversion_to_f_statistic, value;
    BOOLEAN          one_d_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) )
    {
        print( "Usage: %s  output.f_stat  [1] file1 file2 + file3 file4 ..\n",
               argv[0] );
        return( 1 );
    }

    n_surfaces[0] = 0;
    n_surfaces[1] = 0;
    group = 0;

    one_d_flag = FALSE;

    while( get_string_argument( NULL, &filename ) )
    {
        if( equal_strings( filename, "1" ) )
        {
            one_d_flag = TRUE;
            continue;
        }
        else if( equal_strings( filename, "+" ) )
        {
            ++group;
            if( group > 1 )
            {
                print_error( "Too many +\n" );
                return( 1 );
            }
            continue;
        }

        print( "File: %s\n", filename );

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

    ALLOC( avg1_points, n_points );
    ALLOC( avg2_points, n_points );

    for_less( i, 0, n_points )
    {
        fill_Point( avg1_points[i], 0.0, 0.0, 0.0 );
        fill_Point( avg2_points[i], 0.0, 0.0, 0.0 );
    }

    for_less( s, 0, n_surfaces[0] )
    {
        for_less( i, 0, n_points )
            ADD_POINTS( avg1_points[i], avg1_points[i], samples[0][s][i] );
    }

    for_less( s, 0, n_surfaces[1] )
    {
        for_less( i, 0, n_points )
            ADD_POINTS( avg2_points[i], avg2_points[i], samples[1][s][i] );
    }

    for_less( i, 0, n_points )
    {
        SCALE_POINT( avg1_points[i], avg1_points[i], 1.0 / (Real)n_surfaces[0]);
        SCALE_POINT( avg2_points[i], avg2_points[i], 1.0 / (Real)n_surfaces[1]);

        ADD_POINTS( polygons.points[i], avg1_points[i], avg2_points[i] );
        SCALE_POINT( polygons.points[i], polygons.points[i],
                     1.0 / (Real) (n_surfaces[0]+n_surfaces[1]) );
    }

    if( one_d_flag )
        compute_polygon_normals( &polygons );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    ALLOC2D( inv_s, 3, 3 );

    v = n_surfaces[0] + n_surfaces[1] - 2;
    nu = v - N_DIMENSIONS + 1;
    conversion_to_f_statistic = (Real) nu /
                                ((Real) v * (Real) N_DIMENSIONS *
                                 (1.0 / (Real) n_surfaces[0] +
                                  1.0 / (Real) n_surfaces[1]));

    for_less( p, 0, n_points )
    {
        if( one_d_flag )
        {
            for_less( which, 0, 2 )
            {
                sum_x = 0.0;
                sum_xx = 0.0;
                for_less( s, 0, n_surfaces[which] )
                {
                    SUB_POINTS( offset, samples[which][s][p],
                                polygons.points[p] );
                    dist = DOT_VECTORS( offset, polygons.normals[p] );
                    sum_x += dist;
                    sum_xx += dist * dist;
                }

                mean[which] = sum_x / (Real) n_surfaces[which];

                if( n_surfaces[which] == 1 )
                    var[which] = 0.0;
                else
                    var[which] = (sum_xx - sum_x * sum_x /
                       (Real) n_surfaces[which]) / (Real) n_surfaces[which];
            }

            variance = sqrt( ((Real) n_surfaces[0] * var[0] +
                              (Real) n_surfaces[1] * var[1]) /
                             (Real) v );

            value = (mean[0] - mean[1]) /
                       (variance * sqrt( 1.0 / (Real) n_surfaces[0] +
                                         1.0 / (Real) n_surfaces[1] ));
        }
        else
        {
            compute_inv_variance( n_surfaces, samples, p, &avg1_points[p],
                                  &avg2_points[p], inv_s );

            SUB_POINTS( offset, avg1_points[p], avg2_points[p] );

            tx = (Real) Vector_x(offset) * inv_s[0][0] +
                 (Real) Vector_y(offset) * inv_s[1][0] +
                 (Real) Vector_z(offset) * inv_s[2][0];
            ty = (Real) Vector_x(offset) * inv_s[0][1] +
                 (Real) Vector_y(offset) * inv_s[1][1] +
                 (Real) Vector_z(offset) * inv_s[2][1];
            tz = (Real) Vector_x(offset) * inv_s[0][2] +
                 (Real) Vector_y(offset) * inv_s[1][2] +
                 (Real) Vector_z(offset) * inv_s[2][2];

            mahalanobis = tx * (Real) Vector_x(offset) +
                          ty * (Real) Vector_y(offset) +
                          tz * (Real) Vector_z(offset);

            value = mahalanobis * conversion_to_f_statistic;
        }

        (void) output_real( file, value );
        (void) output_newline( file );
    }

    return( 0 );
}

private  void   compute_inv_variance(
    int     n_surfaces[2],
    Point   **samples[2],
    int     p,
    Point   *avg1,
    Point   *avg2,
    Real    **inv_s )
{
    int     s, i, j;
    Real    **variance;
    Real    dx, dy, dz;

    /*--- compute the variance at each of the two nodes */

    ALLOC2D( variance, 3, 3 );

    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
    {
        variance[i][j] = 0.0;
    }

    for_less( s, 0, n_surfaces[0] )
    {
        dx = (Real) Point_x(samples[0][s][p]) - (Real) Point_x(*avg1);
        dy = (Real) Point_y(samples[0][s][p]) - (Real) Point_y(*avg1);
        dz = (Real) Point_z(samples[0][s][p]) - (Real) Point_z(*avg1);

        variance[0][0] += dx * dx;
        variance[0][1] += dx * dy;
        variance[0][2] += dx * dz;
        variance[1][1] += dy * dy;
        variance[1][2] += dy * dz;
        variance[2][2] += dz * dz;
    }

    for_less( s, 0, n_surfaces[1] )
    {
        dx = (Real) Point_x(samples[1][s][p]) - (Real) Point_x(*avg2);
        dy = (Real) Point_y(samples[1][s][p]) - (Real) Point_y(*avg2);
        dz = (Real) Point_z(samples[1][s][p]) - (Real) Point_z(*avg2);

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
        variance[i][j] /= (Real) (n_surfaces[0] + n_surfaces[1] - 2);
    }

    if( !invert_square_matrix( 3, variance, inv_s ) )
        print_error( "Error getting inverse of variance\n" );

    FREE2D( variance );
}
