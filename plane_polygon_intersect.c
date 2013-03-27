#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               src_polygons_filename, dest_lines_filename;
    VIO_STR               axis_name;
    VIO_File_formats         format;
    int                  i, axis;
    VIO_Point                plane_origin;
    VIO_Vector               plane_normal;
    VIO_Real                 position, nx, ny, nz, x, y, z;
    int                  n_objects, n_dest_objects;
    VIO_Colour               colour;
    object_struct        **objects, **dest_objects, *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_polygons_filename ) ||
        !get_string_argument( "", &dest_lines_filename ) )
    {
        print( "Usage: %s  src_polygons  dest_lines x|y|z|[nx ny nz x y z] [position]\n", argv[0] );
        return( 1 );
    }

    (void) get_string_argument( "z", &axis_name );

    if( axis_name[0] >= 'x' && axis_name[0] <= 'z' )
        axis = (int) (axis_name[0] - 'x');
    else if( axis_name[0] >= 'X' && axis_name[0] <= 'Z' )
        axis = (int) (axis_name[0] - 'X');
    else
    {
        axis = -1;
    }

    if( axis < 0 )
    {
        (void) sscanf( axis_name, "%lf", &nx );
        (void) get_real_argument( 0.0, &ny );
        (void) get_real_argument( 0.0, &nz );
        (void) get_real_argument( 0.0, &x );
        (void) get_real_argument( 0.0, &y );
        (void) get_real_argument( 0.0, &z );

        fill_Point( plane_origin, x, y, z );
        fill_Vector( plane_normal, nx, ny, nz );
    }
    else
    {
        (void) get_real_argument( 0.0, &position );

        fill_Point( plane_origin, 0.0, 0.0, 0.0 );
        fill_Vector( plane_normal, 0.0, 0.0, 0.0 );

        Point_coord( plane_origin, axis ) = (VIO_Point_coord_type) position;
        Vector_coord( plane_normal, axis ) = (VIO_Point_coord_type) 1.0;
    }

    if( input_graphics_file( src_polygons_filename,
                             &format, &n_objects, &objects ) != VIO_OK )
        return( 1 );

    n_dest_objects = 0;

    for_less( i, 0, n_objects )
    {
        if( get_object_type( objects[i] ) == POLYGONS )
        {
            object = create_object( LINES );
            intersect_planes_with_polygons( get_polygons_ptr(objects[i]),
                                            &plane_origin, &plane_normal,
                                            get_lines_ptr(object) );

            (void) get_object_colour( objects[i], &colour );
            set_object_colour( object, colour );

            add_object_to_list( &n_dest_objects, &dest_objects, object );
        }
        else if( get_object_type( objects[i] ) == QUADMESH )
        {
            object = create_object( LINES );
            intersect_planes_with_quadmesh( get_quadmesh_ptr(objects[i]),
                                            &plane_origin, &plane_normal,
                                            get_lines_ptr(object) );

            (void) get_object_colour( objects[i], &colour );
            set_object_colour( object, colour );

            add_object_to_list( &n_dest_objects, &dest_objects, object );
        }
    }

    if( output_graphics_file( dest_lines_filename, format, n_dest_objects,
                              dest_objects ) != VIO_OK )
        return( 1 );

    delete_object_list( n_objects, objects );
    delete_object_list( n_dest_objects, dest_objects );

    return( 0 );
}
