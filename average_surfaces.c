#include  <internal_volume_io.h>
#include  <bicpl.h>

#undef SIMPLE_TRANSFORMATION
#define SIMPLE_TRANSFORMATION


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
    polygons_struct       *polygons );

private  void  print_transform(
    Transform   *trans );

int  main(
    int    argc,
    char   *argv[] )
{
    Status           status;
    STRING           filename, output_filename;
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
        !get_real_argument( 0.0, &max_std_dev ) )
    {
        print_error(
          "Usage: %s output.obj hot|gray|spectral min max [input1.obj] [input2.obj] ...\n",
          argv[0] );
        return( 1 );
    }

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

    create_average_polygons( n_surfaces, average_polygons->n_points,
                             points_list, transforms,
                             &colour_coding, average_polygons );

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

    for_less( s, 1, n_surfaces )
    {
        compute_point_to_point_transform( n_points, points[s], points[0],
                                          &transforms[s] );
    }
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

private  void  create_average_polygons(
    int                   n_surfaces,
    int                   n_points,
    Point                 **points,
    Transform             transforms[],
    colour_coding_struct  *colour_coding,
    polygons_struct       *polygons )
{
    Point  avg;
    Real   x, y, z;
    int    p, s;
    Real   variance, dx, dy, dz, std_dev;

    for_less( p, 0, n_points )
    {
        fill_Point( avg, 0.0, 0.0, 0.0 );

        for_less( s, 0, n_surfaces )
        {
            transform_point( &transforms[s],
                             Point_x(points[s][p]),
                             Point_y(points[s][p]),
                             Point_z(points[s][p]), &x, &y, &z );

            Point_x(avg) += x;
            Point_y(avg) += y;
            Point_z(avg) += z;
        }

        SCALE_POINT( polygons->points[p], avg, 1.0/(Real) n_surfaces );

        variance = 0.0;

        for_less( s, 0, n_surfaces )
        {
            transform_point( &transforms[s],
                             Point_x(points[s][p]),
                             Point_y(points[s][p]),
                             Point_z(points[s][p]), &x, &y, &z );
            dx = (x - Point_x(polygons->points[p]));
            dy = (y - Point_y(polygons->points[p]));
            dz = (z - Point_z(polygons->points[p]));
            variance += dx * dx + dy * dy + dz * dz;
        }

        variance /= (Real) (n_surfaces-1);

        std_dev = sqrt( variance );

        polygons->colours[p] = get_colour_code( colour_coding, std_dev );
    }
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
