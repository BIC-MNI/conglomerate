#include  <special_geometry.h>

int  main(
    int    argc,
    char   *argv[] )
{
    char             *input_filename;
    int              i, n_objects;
    File_formats     format;
    object_struct    **object_list;
    VIO_Vector           dir;
    VIO_Point            eye;
    VIO_Real             eye_x, eye_y, eye_z, dir_x, dir_y, dir_z;
    VIO_Real             distance;
    int              obj_index;
    VIO_BOOL          intersects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_real_argument( 0.0, &eye_x ) ||
        !get_real_argument( 0.0, &eye_y ) ||
        !get_real_argument( 0.0, &eye_z ) ||
        !get_real_argument( 0.0, &dir_x ) ||
        !get_real_argument( 0.0, &dir_y ) ||
        !get_real_argument( 0.0, &dir_z ) )
    {
        print_error( "Usage\n" );
        return( 1 );
    }

    fill_Point( eye, eye_x, eye_y, eye_z );
    fill_Vector( dir, dir_x, dir_y, dir_z );
    NORMALIZE_VECTOR( dir, dir );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK )
        return( 1 );

    intersects = intersect_ray_with_object( &eye, &dir, object_list[0],
                                            &obj_index, &distance, NULL );

    print( "%d: %g\n", intersects, distance );

    return( 0 );
}
