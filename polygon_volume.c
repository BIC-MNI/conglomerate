#include  <special_geometry.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status           status;
    char             *input_filename;
    polygons_struct  *polygons;
    int              i, p, n_objects;
    Real             x, y, z;
    File_formats     format;
    Transform        transform, x_rot, y_rot, z_rot;
    object_struct    **object_list;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( status == OK && get_object_type(object_list[i]) == POLYGONS )
        {
            polygons = get_polygons_ptr(object_list[i]);

            print( "Volume: %g\n", get_closed_polyhedron_volume(polygons));

            make_rotation_transform( get_random_0_to_1() * 2.0 * PI, X, &x_rot);
            make_rotation_transform( get_random_0_to_1() * 2.0 * PI, Y, &y_rot);
            make_rotation_transform( get_random_0_to_1() * 2.0 * PI, Z, &z_rot);

            concat_transforms( &transform, &x_rot, &y_rot );
            concat_transforms( &transform, &transform, &z_rot );

            for_less( p, 0, polygons->n_points )
            {
                transform_point( &transform,
                                 Point_x(polygons->points[p]),
                                 Point_y(polygons->points[p]),
                                 Point_z(polygons->points[p]),
                                 &x, &y, &z );
                fill_Point( polygons->points[p], x, y, z );
            }

            print( "Rotated Volume: %g\n",
                   get_closed_polyhedron_volume(polygons));
        }
    }

    if( status == OK )
        delete_object_list( n_objects, object_list );

    return( status != OK );
}
