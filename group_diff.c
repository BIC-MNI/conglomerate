#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void   compute_inv_variance(
    int     n_surfaces[2],
    Point   **samples[2],
    int     p,
    Real    **inv_s );

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           filename, output_filename;
    int              i, n_objects, n_surfaces[2], group, n_points, s, p;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  polygons;
    Point            *points, *copy_points, **samples[2];
    Point            *avg1_points, *avg2_points;
    Vector           offset;
    Real             **inv_s, tx, ty, tz, mahalanobis;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) )
    {
        print( "Usage: %s  output.variance  file1 file2 + file3 file4 ..\n",
               argv[0] );
        return( 1 );
    }

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
        ADD_POINTS( polygons.points[i], avg1_points[i], avg2_points[i] );

        SCALE_POINT( polygons.points[i], polygons.points[i],
                     1.0 / (Real) (n_surfaces[0] + n_surfaces[1]) );
        SCALE_POINT( avg1_points[i], avg1_points[i],
                     1.0 / (Real) n_surfaces[0] );
        SCALE_POINT( avg2_points[i], avg2_points[i],
                     1.0 / (Real) n_surfaces[1] );
    }

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    ALLOC2D( inv_s, 3, 3 );

    for_less( p, 0, polygons.n_points )
    {
        compute_inv_variance( n_surfaces, samples, p, inv_s );

        SUB_POINTS( offset, avg1_points[p], avg2_points[p] );

        tx = Vector_x(offset) * inv_s[0][0] +
             Vector_y(offset) * inv_s[1][0] +
             Vector_z(offset) * inv_s[2][0];
        ty = Vector_x(offset) * inv_s[0][1] +
             Vector_y(offset) * inv_s[1][1] +
             Vector_z(offset) * inv_s[2][1];
        tz = Vector_x(offset) * inv_s[0][2] +
             Vector_y(offset) * inv_s[1][2] +
             Vector_z(offset) * inv_s[2][2];

        mahalanobis = tx * Vector_x(offset) +
                      ty * Vector_y(offset) +
                      tz * Vector_z(offset);


        (void) output_real( file, mahalanobis );
        (void) output_newline( file );
    }

    return( 0 );
}

private  void   compute_inv_variance(
    int     n_surfaces[2],
    Point   **samples[2],
    int     p,
    Real    **inv_s )
{
    int     c, s, i, j;
    Real    **variance;
    Real    mean[2][3];
    Real    dx, dy, dz;

    for_less( c, 0, 3 )
    {
        mean[0][c] = 0.0;
        mean[1][c] = 0.0;
    }

    for_less( s, 0, n_surfaces[0] )
    {
        mean[0][X] += Point_x(samples[0][s][p]);
        mean[0][Y] += Point_y(samples[0][s][p]);
        mean[0][Z] += Point_z(samples[0][s][p]);
    }

    for_less( s, 0, n_surfaces[1] )
    {
        mean[1][X] += Point_x(samples[1][s][p]);
        mean[1][Y] += Point_y(samples[1][s][p]);
        mean[1][Z] += Point_z(samples[1][s][p]);
    }

    for_less( c, 0, 3 )
    {
        mean[0][c] /= (Real) n_surfaces[0];
        mean[1][c] /= (Real) n_surfaces[1];
    }

    /*--- compute the variance at each of the two nodes */

    ALLOC2D( variance, 3, 3 );

    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
    {
        variance[i][j] = 0.0;
    }

    for_less( s, 0, n_surfaces[0] )
    {
        dx = Point_x(samples[0][s][p]) - mean[0][0];
        dy = Point_y(samples[0][s][p]) - mean[0][1];
        dz = Point_z(samples[0][s][p]) - mean[0][2];

        variance[0][0] += dx * dx;
        variance[0][1] += dx * dy;
        variance[0][2] += dx * dz;
        variance[1][1] += dy * dy;
        variance[1][2] += dy * dz;
        variance[2][2] += dz * dz;
    }

    for_less( s, 0, n_surfaces[1] )
    {
        dx = Point_x(samples[1][s][p]) - mean[1][0];
        dy = Point_y(samples[1][s][p]) - mean[1][1];
        dz = Point_z(samples[1][s][p]) - mean[1][2];

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
