#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

private  void   map_unit_sphere_to_2d(
    Point   *point,
    Real    *x,
    Real    *y );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s surface.obj  input.obj output.obj\n\
\n\
     Maps an object on a surface to a flat sheet.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               surface_filename, input_filename, output_filename;
    File_formats         format;
    polygons_struct      *surface, unit_sphere;
    int                  n_objects;
    int                  n_points, p, obj, n_surf_objects;
    Point                *points;
    Real                 x, y;
    object_struct        **objects, **surf_objects;
    Point                unit_point, centre;
    progress_struct      progress;

    /*--- get the arguments from the command line */

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    /*--- input the surface */

     if( input_graphics_file( surface_filename, &format, &n_surf_objects,
                              &surf_objects )
         != OK )
        return( 1 );

    /*--- check that the surface file contains a polyhedron */

    if( n_surf_objects != 1 || get_object_type( surf_objects[0] ) != POLYGONS )
    {
        print( "Surface file must contain 1 polygons object.\n" );
        return( 1 );
    }

    /*--- input the objects */

    if( input_graphics_file( input_filename, &format, &n_objects, &objects )
         != OK )
        return( 1 );

    /*--- get a pointer to the surface */

    surface = get_polygons_ptr( surf_objects[0] );

    check_polygons_neighbours_computed( surface );

    /*--- create a unit sphere with same number of triangles as skin surface */

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               surface->n_items, &unit_sphere );

    create_polygons_bintree( surface,
                             ROUND( (Real) surface->n_items * BINTREE_FACTOR ));

    for_less( obj, 0, n_objects )
    {
        n_points = get_object_points( objects[obj], &points );

        initialize_progress_report( &progress, FALSE, n_points, "Mapping" );

        for_less( p, 0, n_points )
        {
            map_point_to_unit_sphere( surface, &points[p],
                                      &unit_sphere, &unit_point );

            map_unit_sphere_to_2d( &unit_point, &x, &y );

            fill_Point( points[p], x, y, 0.0 );

            update_progress_report( &progress, p + 1 );
        }

        terminate_progress_report( &progress );
    }

    (void) output_graphics_file( output_filename, format, n_objects, objects );

    return( 0 );
}

private  void   map_unit_sphere_to_2d(
    Point   *point,
    Real    *x,
    Real    *y )
{
    Real  xp, yp, zp;
    Real  angle_around, angle_up;

    xp = (Real) Point_x( *point );
    yp = (Real) Point_y( *point );
    zp = (Real) Point_z( *point );

    angle_around = 2.0 * PI - compute_clockwise_rotation( xp, yp );


    angle_up = compute_clockwise_rotation( sqrt( 1.0 - zp * zp ), zp );

    if( angle_up > PI )
        angle_up -= 2.0 * PI;

    *x = angle_around / 2.0 / PI;
    *y = (PI / 2.0 - angle_up) / PI;
}
