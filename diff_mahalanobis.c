#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  MAX_ITERS   1000
#define  TOLERANCE   1.0e-5

private  Real  get_z_score(
    Point     *avg1,
    Real      **inv_variance1,
    Point     *avg2,
    Real      **inv_variance2 );

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *output_file, *variance_file;
    STRING           surface1_filename, variance1_filename;
    STRING           surface2_filename, variance2_filename, output_filename;
    int              p, i, j;
    int              n_objects1, n_objects2, n_points;
    File_formats     format;
    object_struct    **object_list1, **object_list2;
    Point            *points1, *points2;
    Real             ***variance1, ***variance2, z;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface1_filename ) ||
        !get_string_argument( NULL, &variance1_filename ) ||
        !get_string_argument( NULL, &surface2_filename ) ||
        !get_string_argument( NULL, &variance2_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
          "Usage: %s  avg1.obj variance1.txt avg2.obj variance2.txt  output\n",
            argv[0] );
        return( 1 );
    }

    if( input_graphics_file( surface1_filename, &format, &n_objects1,
                             &object_list1 ) != OK || n_objects1 != 1 )
    {
        print( "Error reading %s.\n", surface1_filename );
        return( 1 );
    }

    n_points = get_object_points( object_list1[0], &points1 );

    ALLOC3D( variance1, n_points, 3, 3 );

    if( open_file( variance1_filename, READ_FILE, ASCII_FORMAT, &variance_file )
        != OK )
        return( 1 );

    for_less( p, 0, n_points )
    {
        for_less( i, 0, 3 )
        for_less( j, 0, 3 )
            if( input_real( variance_file, &variance1[p][i][j] ) != OK )
                return( 1 );
    }

    (void) close_file( variance_file );

    if( input_graphics_file( surface2_filename, &format, &n_objects2,
                             &object_list2 ) != OK || n_objects2 != 1 )
    {
        print( "Error reading %s.\n", surface2_filename );
        return( 1 );
    }

    if( get_object_points( object_list2[0], &points2 ) != n_points )
    {
        print( "Invalid number of points in %s\n", surface2_filename );
        return( 1 );
    }

    ALLOC3D( variance2, n_points, 3, 3 );

    if( open_file( variance2_filename, READ_FILE, ASCII_FORMAT, &variance_file )
        != OK )
        return( 1 );

    for_less( p, 0, n_points )
    {
        for_less( i, 0, 3 )
        for_less( j, 0, 3 )
            if( input_real( variance_file, &variance2[p][i][j] ) != OK )
                return( 1 );
    }

    (void) close_file( variance_file );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &output_file )
        != OK )
        return( 1 );

    for_less( p, 0, n_points )
    {
        z = get_z_score( &points1[p], variance1[p], &points2[p], variance2[p] );

        if( output_real( variance_file, z ) != OK ||
            output_newline( variance_file ) != OK )
            return( 1 );
    }

    (void) close_file( output_file );

    return( 0 );
}

private  Real  compute_mahalanobis_distance(
    Real    avg[],
    Real    **inv_var,
    Real    point[],
    Real    deriv[] )
{
    Real   x, y, z, tx, ty, tz, m;

    x = point[0] - avg[0];
    y = point[1] - avg[1];
    z = point[2] - avg[2];

    tx = x * inv_var[0][0] + y * inv_var[1][0] + z * inv_var[2][0];
    ty = x * inv_var[0][1] + y * inv_var[1][1] + z * inv_var[2][1];
    tz = x * inv_var[0][2] + y * inv_var[1][2] + z * inv_var[2][2];

    m = x * tx + y * ty + z * tz;

    if( deriv != NULL )
    {
        deriv[0] = 2.0 * tx;
        deriv[1] = 2.0 * ty;
        deriv[2] = 2.0 * tz;
    }

    return( m );
}

typedef  struct
{
    Real  avg1[3];
    Real  **inv_variance1;
    Real  avg2[3];
    Real  **inv_variance2;
} min_data_struct;

private  Real  min_function(
    Real   parameters[],
    void   *void_data )
{
    Real              z1, z2, diff, deriv1[3], deriv2[3], close;
    Real              f1_dot_f1, f1_dot_f2, f2_dot_f2;
    min_data_struct   *data;

    data = (min_data_struct *) void_data;

    z1 = compute_mahalanobis_distance( data->avg1, data->inv_variance1,
                                       parameters, deriv1 );
    z2 = compute_mahalanobis_distance( data->avg2, data->inv_variance2,
                                       parameters, deriv2 );
    diff = z1 - z2;
    close = diff * diff;

    f1_dot_f1 = deriv1[0] * deriv1[0] + deriv1[1] * deriv1[1] +
                deriv1[2] * deriv1[2];
    f2_dot_f2 = deriv2[0] * deriv2[0] + deriv2[1] * deriv2[1] +
                deriv2[2] * deriv2[2];
    f1_dot_f2 = deriv1[0] * deriv2[0] + deriv1[1] * deriv2[1] +
                deriv1[2] * deriv2[2];

    diff = f1_dot_f2 + sqrt( f1_dot_f1 * f2_dot_f2 );

    close += diff * diff;

    return( diff * diff );
}

private  Real  get_z_score(
    Point     *avg1,
    Real      **inv_variance1,
    Point     *avg2,
    Real      **inv_variance2 )
{
    int               c;
    Real              initial[3], initial_steps[3], solution[3], z;
    min_data_struct   data;

    for_less( c, 0, 3 )
    {
        initial[c] = ((Real) Point_coord(*avg1,c) +
                      (Real) Point_coord(*avg2,c)) / 2.0;
        initial_steps[c] = 1.0;
        data.avg1[c] = (Real) Point_coord(*avg1,c);
        data.inv_variance1 = inv_variance1;
        data.avg2[c] = (Real) Point_coord(*avg2,c);
        data.inv_variance2 = inv_variance2;
    }

    gradient_steps_minimize_function( 3, initial, initial_steps,
                                      min_function, (void *) &data,
                                      3, MAX_ITERS, TOLERANCE, solution );

    z = compute_mahalanobis_distance( data.avg1, inv_variance1, solution, NULL);

#ifdef DEBUG
    value = min_function( solution, (void *) &data );

    print( "value: %g\n", value );
#endif

    return( z );
}
