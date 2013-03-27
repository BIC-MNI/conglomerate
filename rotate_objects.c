#include  <def_mni.h>
#include  <def_module.h>

public  VIO_Status  process_object(
    object_struct  *object );

private  void  transform_object(
    Transform      *transform,
    object_struct  *object )
{
    int    i, n_points;
    VIO_Point  *points;
    VIO_Real   x, y, z;

    n_points = get_object_points( object, &points );

    for_less( i, 0, n_points )
    {
        transform_point( transform,
                         Point_x(points[i]),
                         Point_y(points[i]),
                         Point_z(points[i]), &x, &y, &z );

        fill_Point( points[i], x, y, z );
    }
}

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_Status         status;
    char           *input_filename, *output_filename;
    VIO_Real           angle;
    int            i, n_objects;
    Transform      transform;
    File_formats   format;
    object_struct  **object_list;

    status = VIO_OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_real_argument( 0.0, &angle ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );

    make_rotation_transform( angle * DEG_TO_RAD, Z, &transform );

    status = input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list );

    if( status == VIO_OK )
        print( "Objects input.\n" );

    if( status == VIO_OK )
    {
        for_less( i, 0, n_objects )
        {
            if( status == VIO_OK )
                transform_object( &transform, object_list[i] );
        }

        if( status == VIO_OK )
            print( "Objects processed.\n" );
    }

    if( status == VIO_OK )
        status = output_graphics_file( output_filename, format,
                                       n_objects, object_list );

    if( status == VIO_OK )
        delete_object_list( n_objects, object_list );

    if( status == VIO_OK )
        print( "Objects output.\n" );

    return( status != VIO_OK );
}
