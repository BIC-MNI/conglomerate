#include  <internal_volume_io.h>
#include  <bicpl.h>

#undef SIMPLE_TRANSFORMATION
#define SIMPLE_TRANSFORMATION

#define USE_IDENTITY_TRANSFORMS


#define  GRAY_STRING       "gray"
#define  HOT_STRING        "hot"
#define  SPECTRAL_STRING   "spectral"
#define  RED_STRING        "red"
#define  GREEN_STRING      "green"
#define  BLUE_STRING       "blue"


private  void  compute_transforms(
    int        n_surfaces,
    int        n_points,
    Point      **points,
    Transform  transforms[] );

private  void  create_average_polygons(
    int                   n_surfaces,
    int                   n_points,
    Point                 **points,
    Transform             transforms[],
    colour_coding_struct  *colour_coding,
    FILE                  *dump_file,
    polygons_struct       *polygons );

private  void  print_transform(
    Transform   *trans );

int  main(
    int    argc,
    char   *argv[] )
{
    Status           status;
    FILE             *file;
    STRING           filename, output_filename, dump_filename;
    int              i, n_objects, n_surfaces;
    Colour           *colours;
    File_formats     format;
    object_struct    *out_object;
    object_struct    **object_list;
    polygons_struct  *polygons, *average_polygons;
    Point            **points_list;
    Transform        *transforms;
    Colour_coding_types  coding_type;
    colour_coding_struct colour_coding;
    STRING               coding_type_string;
    Real                 min_std_dev, max_std_dev;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &coding_type_string ) ||
        !get_real_argument( 0.0, &min_std_dev ) ||
        !get_real_argument( 0.0, &max_std_dev ) ||
        !get_string_argument( NULL, &dump_filename ) )
    {
        print_error(
          "Usage: %s output.obj hot|gray|spectral min max none|dump_file\n",
                  argv[0] );
        print_error( "         [input1.obj] [input2.obj] ...\n" );
        return( 1 );
    }

    if( equal_strings( dump_filename, "none" ) )
        dump_filename = NULL;

    if( equal_strings( coding_type_string, GRAY_STRING ) )
        coding_type = GRAY_SCALE;
    else if( equal_strings( coding_type_string, HOT_STRING ) )
        coding_type = HOT_METAL;
    else if( equal_strings( coding_type_string, SPECTRAL_STRING ) )
        coding_type = SPECTRAL;
    else if( equal_strings( coding_type_string, RED_STRING ) )
        coding_type = RED_COLOUR_MAP;
    else if( equal_strings( coding_type_string, GREEN_STRING ) )
        coding_type = GREEN_COLOUR_MAP;
    else if( equal_strings( coding_type_string, BLUE_STRING ) )
        coding_type = BLUE_COLOUR_MAP;
    else
    {
        print( "Invalid coding type: %s\n", coding_type_string );
        return( 1 );
    }

    initialize_colour_coding( &colour_coding, coding_type,
                              BLACK, WHITE, min_std_dev, max_std_dev );

    n_surfaces = 0;

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

            (void) get_object_colours( out_object, &colours );
            REALLOC( colours, polygons->n_points );
            set_object_colours( out_object, colours );
            average_polygons->colour_flag = PER_VERTEX_COLOURS;
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

    if( dump_filename != NULL )
    {
        if( open_file( dump_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );
    }
    else
        file = NULL;

    create_average_polygons( n_surfaces, average_polygons->n_points,
                             points_list, transforms,
                             &colour_coding, file, average_polygons );

    if( dump_filename != NULL )
        (void) close_file( file );

    compute_polygon_normals( average_polygons );

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       1, &out_object );

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}

#ifdef SIMPLE_TRANSFORMATION

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
        coords[p][X] = Point_x(src_points[p]);
        coords[p][Y] = Point_y(src_points[p]);
        coords[p][Z] = Point_z(src_points[p]);
    }

    make_identity_transform( transform );

    for_less( dim, 0, N_DIMENSIONS )
    {
        for_less( p, 0, n_points )
            target[p] = Point_coord(dest_points[p],dim);

        least_squares( n_points, N_DIMENSIONS, coords, target, coefs );

        Transform_elem( *transform, dim, 0 ) = coefs[1];
        Transform_elem( *transform, dim, 1 ) = coefs[2];
        Transform_elem( *transform, dim, 2 ) = coefs[3];
        Transform_elem( *transform, dim, 3 ) = coefs[0];
    }

    FREE( target );
    FREE2D( coords );
}

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

private  Real  get_std_points(
    int                   n_surfaces,
    Point                 samples[],
    Point                 *centroid )
{
    int    i, j, s;
    Real   variance, dx, dy, dz, std_dev, tdx, tdy, tdz;
    Real   sums[N_DIMENSIONS][N_DIMENSIONS];
    Real   **matrix, **inverse;

    for_less( i, 0, N_DIMENSIONS )
        for_less( j, i, N_DIMENSIONS )
            sums[i][j] = 0.0;

    for_less( s, 0, n_surfaces )
    {
        dx = Point_x(samples[s]) - Point_x(*centroid);
        dy = Point_y(samples[s]) - Point_y(*centroid);
        dz = Point_z(samples[s]) - Point_z(*centroid);

        sums[0][0] += dx * dx;
        sums[0][1] += dx * dy;
        sums[0][2] += dx * dz;
        sums[1][1] += dy * dy;
        sums[1][2] += dy * dz;
        sums[2][2] += dz * dz;
    }

    ALLOC2D( matrix, N_DIMENSIONS, N_DIMENSIONS );
    ALLOC2D( inverse, N_DIMENSIONS, N_DIMENSIONS );

    for_less( i, 0, N_DIMENSIONS )
    for_less( j, i, N_DIMENSIONS )
    {
        matrix[i][j] = sums[i][j];
        matrix[j][i] = sums[i][j];
    }

    if( !invert_square_matrix( N_DIMENSIONS, matrix, inverse ) )
    {
        print_error( "Setting std dev to 0.\n" );
        return( 0.0 );
    }

    variance = 0.0;
    for_less( s, 0, n_surfaces )
    {
        dx = Point_x(samples[s]) - Point_x(*centroid);
        dy = Point_y(samples[s]) - Point_y(*centroid);
        dz = Point_z(samples[s]) - Point_z(*centroid);

        tdx = inverse[0][0] * dx + inverse[0][1] * dy + inverse[0][2] * dz;
        tdy = inverse[1][0] * dx + inverse[1][1] * dy + inverse[1][2] * dz;
        tdz = inverse[2][0] * dx + inverse[2][1] * dy + inverse[2][2] * dz;

        variance += tdx * tdx + tdy * tdy + tdz * tdz;
    }

    variance /= (Real) (n_surfaces-1);

    std_dev = sqrt( variance );

    FREE2D( matrix );
    FREE2D( inverse );

    return( std_dev );
}

private  void  create_average_polygons(
    int                   n_surfaces,
    int                   n_points,
    Point                 **points,
    Transform             transforms[],
    colour_coding_struct  *colour_coding,
    FILE                  *dump_file,
    polygons_struct       *polygons )
{
    Real   x, y, z;
    int    p, s;
    Real   std_dev;
    Point  *samples;

    ALLOC( samples, n_surfaces );

    for_less( p, 0, n_points )
    {
        for_less( s, 0, n_surfaces )
        {
            transform_point( &transforms[s],
                             Point_x(points[s][p]),
                             Point_y(points[s][p]),
                             Point_z(points[s][p]), &x, &y, &z );

            fill_Point( samples[s], x, y, z );
        }

        get_points_centroid( n_surfaces, samples, &polygons->points[p] );

        std_dev = get_std_points( n_surfaces, samples, &polygons->points[p] );

        if( dump_file != NULL )
        {
            (void) output_real( dump_file, std_dev );
            (void) output_newline( dump_file );
        }

        polygons->colours[p] = get_colour_code( colour_coding, std_dev );
    }

    FREE( samples );
}

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
