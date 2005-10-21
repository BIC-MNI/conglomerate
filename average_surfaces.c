#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

#undef SIMPLE_TRANSFORMATION
#define SIMPLE_TRANSFORMATION

#define USE_IDENTITY_TRANSFORMS


private  void  compute_transforms(
    int        n_surfaces,
    int        n_points,
    Point      **points,
    Transform  transforms[] );

private  void  create_average_polygons(
    int                   n_surfaces,
    int                   n_groups,
    int                   n_points,
    Point                 **points,
    Transform             transforms[],
    FILE                  *rms_file,
    FILE                  *variance_file,
    polygons_struct       *polygons );

#ifdef PRINT_TRANSFORMS
private  void  print_transform(
    Transform   *trans );
#endif

int  main(
    int    argc,
    char   *argv[] )
{
    Status           status;
    FILE             *rms_file, *variance_file;
    STRING           filename, output_filename;
    STRING           rms_filename, variance_filename;
    int              i, n_objects, n_surfaces, n_groups;
    File_formats     format;
    object_struct    *out_object;
    object_struct    **object_list;
    polygons_struct  *polygons, *average_polygons;
    Point            **points_list;
    Transform        *transforms;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &rms_filename ) ||
        !get_string_argument( NULL, &variance_filename ) ||
        !get_int_argument( 1, &n_groups ) )
    {
        print_error(
          "Usage: %s output.obj  none|rms_file  none|variance_file n_groups\n",
                  argv[0] );
        print_error( "         [input1.obj] [input2.obj] ...\n" );
        return( 1 );
    }

    if( equal_strings( rms_filename, "none" ) )
        rms_filename = NULL;
    if( equal_strings( variance_filename, "none" ) )
        variance_filename = NULL;

    n_surfaces = 0;
    points_list = NULL;

    out_object = create_object( POLYGONS );
    average_polygons = get_polygons_ptr(out_object);

    while( get_string_argument( NULL, &filename ) )
    {
        if( input_graphics_file( filename, &format, &n_objects,
                                 &object_list ) != OK )
        {
            print( "Couldn't read %s.\n", filename );
            return( 1 );
        }

        if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
        {
            print( "Invalid object in file.\n" );
            return( 1 );
        }

        polygons = get_polygons_ptr( object_list[0] );

        if( n_surfaces == 0 )
        {
            copy_polygons( polygons, average_polygons );
        }
        else if( !polygons_are_same_topology( average_polygons, polygons ) )
        {
            print( "Invalid polygons topology in file.\n" );
            return( 1 );
        }

        SET_ARRAY_SIZE( points_list, n_surfaces, n_surfaces+1, 1 );
        ALLOC( points_list[n_surfaces], polygons->n_points );

        for_less( i, 0, polygons->n_points )
            points_list[n_surfaces][i] = polygons->points[i];

        ++n_surfaces;

        print( "%d:  %s\n", n_surfaces, filename );

        delete_object_list( n_objects, object_list );
    }

    ALLOC( transforms, n_surfaces );

    compute_transforms( n_surfaces, average_polygons->n_points,
                        points_list, transforms );

#ifdef  PRINT_TRANSFORMS
    for_less( i, 0, n_surfaces )
        print_transform( &transforms[i] );
#endif

    if( rms_filename != NULL )
    {
        if( open_file( rms_filename, WRITE_FILE, ASCII_FORMAT, &rms_file )!= OK)
            return( 1 );
    }
    else
        rms_file = NULL;

    if( variance_filename != NULL )
    {
        if( open_file( variance_filename, WRITE_FILE, ASCII_FORMAT,
                       &variance_file )!= OK)
            return( 1 );
    }
    else
        variance_file = NULL;

    create_average_polygons( n_surfaces, n_groups, average_polygons->n_points,
                             points_list, transforms,
                             rms_file, variance_file,
                             average_polygons );

    if( rms_filename != NULL )
        (void) close_file( rms_file );

    if( variance_filename != NULL )
        (void) close_file( variance_file );

    compute_polygon_normals( average_polygons );

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       1, &out_object );

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}

#ifdef SIMPLE_TRANSFORMATION

#ifndef  USE_IDENTITY_TRANSFORMS
private  void  compute_point_to_point_transform(
    int        n_points,
    Point      src_points[],
    Point      dest_points[],
    Transform  *transform )
{
    int    p, dim;
    Real   **coords, *target, coefs[4];

    ALLOC2D( coords, n_points, 3 );
    ALLOC( target, n_points );

    for_less( p, 0, n_points )
    {
        coords[p][X] = (Real) Point_x(src_points[p]);
        coords[p][Y] = (Real) Point_y(src_points[p]);
        coords[p][Z] = (Real) Point_z(src_points[p]);
    }

    make_identity_transform( transform );

    for_less( dim, 0, N_DIMENSIONS )
    {
        for_less( p, 0, n_points )
            target[p] = (Real) Point_coord(dest_points[p],dim);

        least_squares( n_points, N_DIMENSIONS, coords, target, coefs );

        Transform_elem( *transform, dim, 0 ) = coefs[1];
        Transform_elem( *transform, dim, 1 ) = coefs[2];
        Transform_elem( *transform, dim, 2 ) = coefs[3];
        Transform_elem( *transform, dim, 3 ) = coefs[0];
    }

    FREE( target );
    FREE2D( coords );
}
#endif

private  void  compute_transforms(
    int        n_surfaces,
    int        n_points,
    Point      **points,
    Transform  transforms[] )
{
    int   s;

    make_identity_transform( &transforms[0] );

#ifdef  USE_IDENTITY_TRANSFORMS
    for_less( s, 1, n_surfaces )
        make_identity_transform( &transforms[s] );
#else
    for_less( s, 1, n_surfaces )
    {
        compute_point_to_point_transform( n_points, points[s], points[0],
                                          &transforms[s] );
    }
#endif
}
#else

private  void  compute_transforms(
    int        n_surfaces,
    int        n_points,
    Point      **points,
    Transform  transforms[] )
{
    int                    dim, s, i, j;
    linear_least_squares   lsq;
    Real                   *coefs, n, constant;

    ALLOC( coefs, (n_surfaces-1) * 4 );

    for_less( s, 0, n_surfaces )
        make_identity_transform( &transforms[s] );

    n = (Real) n_surfaces;

    for_less( dim, 0, N_DIMENSIONS )
    {
        initialize_linear_least_squares( &lsq, (n_surfaces-1) * 4 );

        for_less( i, 0, n_points )
        {
            for_less( s, 0, n_surfaces )
            {
                for_less( j, 0, (n_surfaces-1)*4 )
                    coefs[j] = 0.0;
                constant = 0.0;

                if( s == 0 )
                {
                    constant = -Point_coord(points[s][i],dim);
                }
                else
                {
                    coefs[IJ(s-1,0,4)] = Point_x(points[s][i]);
                    coefs[IJ(s-1,1,4)] = Point_y(points[s][i]);
                    coefs[IJ(s-1,2,4)] = Point_z(points[s][i]);
                    coefs[IJ(s-1,3,4)] = 1.0;
                }

                for_less( j, 0, n_surfaces )
                {
                    if( j == 0 )
                    {
                        constant -= -Point_coord(points[j][i],dim) / n;
                    }
                    else
                    {
                        coefs[IJ(j-1,0,4)] -= Point_x(points[j][i]) / n;
                        coefs[IJ(j-1,1,4)] -= Point_y(points[j][i]) / n;
                        coefs[IJ(j-1,2,4)] -= Point_z(points[j][i]) / n;
                        coefs[IJ(j-1,3,4)] -= 1.0 / n;
                    }
                }

                add_to_linear_least_squares( &lsq, coefs, constant );
            }
        }

        (void) get_linear_least_squares_solution( &lsq, coefs );

        for_less( s, 1, n_surfaces )
        {
            for_less( j, 0, N_DIMENSIONS+1 )
                Transform_elem(transforms[s],dim,j) = coefs[IJ(s-1,j,4)];
        }

        delete_linear_least_squares( &lsq );
    }

    FREE( coefs );
}

#endif

private  Real  get_rms_points(
    int                   n_surfaces,
    int                   n_groups,
    Point                 samples[],
    Point                 *centroid,
    Real                  inv_variance[3][3] )
{
    int    i, j, s;
    Real   rms, dx, dy, dz;
    Real   variance[3][3], inverse[3][3];

    rms = 0.0;

    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
        variance[i][j] = 0.0;

    for_less( s, 0, n_surfaces )
    {
        dx = (Real) Point_x(samples[s]) - (Real) Point_x(*centroid);
        dy = (Real) Point_y(samples[s]) - (Real) Point_y(*centroid);
        dz = (Real) Point_z(samples[s]) - (Real) Point_z(*centroid);

        variance[0][0] += dx * dx;
        variance[0][1] += dx * dy;
        variance[0][2] += dx * dz;
        variance[1][1] += dy * dy;
        variance[1][2] += dy * dz;
        variance[2][2] += dz * dz;

        rms += dx * dx + dy * dy + dz * dz;
    }

    variance[1][0] = variance[0][1];
    variance[2][0] = variance[0][2];
    variance[2][1] = variance[1][2];

    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
        variance[i][j] /= (Real) (n_surfaces-n_groups);

    if( !invert_square_matrix( 3, (Real**)variance, (Real**)inverse ) ) {
        print_error( "Error getting inverse of variance\n" );
    }

    for_less( i, 0, 3 )
    for_less( j, 0, 3 )
        inv_variance[i][j] = inverse[i][j];

    rms /= (Real) n_surfaces;

    rms = sqrt( rms );

    return( rms );
}

private  void  create_average_polygons(
    int                   n_surfaces,
    int                   n_groups,
    int                   n_points,
    Point                 **points,
    Transform             transforms[],
    FILE                  *rms_file,
    FILE                  *variance_file,
    polygons_struct       *polygons )
{
    Real   x, y, z;
    int    i, j, p, s;
    Real   rms, inv_variance[3][3];
    Point  *samples;

    ALLOC( samples, n_surfaces );

    for_less( p, 0, n_points )
    {
        for_less( s, 0, n_surfaces )
        {
            transform_point( &transforms[s],
                             (Real) Point_x(points[s][p]),
                             (Real) Point_y(points[s][p]),
                             (Real) Point_z(points[s][p]), &x, &y, &z );

            fill_Point( samples[s], x, y, z );
        }

        get_points_centroid( n_surfaces, samples, &polygons->points[p] );

        if( rms_file != NULL || variance_file != NULL ) {
          rms = get_rms_points( n_surfaces, n_groups,
                                samples, &polygons->points[p], inv_variance );

          if( rms_file != NULL ) {
            (void) output_real( rms_file, rms );
            (void) output_newline( rms_file );
          } 
          if( variance_file != NULL ) {
            for_less( i, 0, 3 )
            for_less( j, 0, 3 )
                (void) output_real( variance_file, inv_variance[i][j] );
            (void) output_newline( variance_file );
          }
        }
    }

    FREE( samples );
}

#ifdef PRINT_TRANSFORMS
private  void  print_transform(
    Transform   *trans )
{
    int   i, j;

    for_less( i, 0, N_DIMENSIONS )
    {
        for_less( j, 0, N_DIMENSIONS+1 )
            print( " %12g", Transform_elem(*trans,i,j) );
        print( "\n" );
    }

    print( "\n" );
}
#endif
