#include  <module.h>

int  main(
    int   argc,
    char   *argv[] )
{
    int                 n_triangles;
    VIO_Point               centre, p1, p2;
    polygons_struct     unit1, unit2;

    initialize_argument_processing( argc, argv );

    (void) get_int_argument( 512, &n_triangles );

    fill_Point( centre, 0.0, 0.0, 0.0 );
    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0, n_triangles, &unit1 );
    create_tetrahedral_sphere( &centre, 2.0, 2.0, 2.0, n_triangles, &unit2 );

    while( input_float( stdin, &Point_x(p1) ) == VIO_OK &&
           input_float( stdin, &Point_y(p1) ) == VIO_OK &&
           input_float( stdin, &Point_z(p1) ) == VIO_OK )
    {
        map_point_to_unit_sphere( &unit2, &p1, &unit1, &p2 );
        print( "%g %g %g  --> %g %g %g\n",
               Point_x(p1), Point_y(p1), Point_z(p1),
               Point_x(p2), Point_y(p2), Point_z(p2) );
    }

    return( 0 );
}
