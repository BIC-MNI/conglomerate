#include  <special_geometry.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status           status;
    char             *input_filename, *output_filename;
    int              i, n_objects;
    polygons_struct  clipped;
    File_formats     format;
    object_struct    **object_list;
    static  Vector   plane_normal = { 0.0, 0.0, 1.0 };
    static  Real     plane_constant = 0.0;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );

    status = input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list );

    if( status == OK )
        print( "Objects input.\n" );

    if( status == OK )
    {
        for_less( i, 0, n_objects )
        {
            if( status == OK && get_object_type(object_list[i]) )
            {
                clip_polygons_to_plane( get_polygons_ptr(object_list[i]),
                                        &plane_normal, plane_constant,
                                        &clipped );
                delete_polygons( get_polygons_ptr(object_list[i]) );
                *get_polygons_ptr(object_list[i]) = clipped;
            }
        }

        if( status == OK )
            print( "Objects processed.\n" );
    }

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       n_objects, object_list );

    if( status == OK )
        delete_object_list( n_objects, object_list );

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}
