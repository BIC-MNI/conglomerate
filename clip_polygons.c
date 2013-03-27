#include  <special_geometry.h>

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_Status           status;
    char             *input_filename, *output_filename;
    int              i, n_objects, clip_status;
    VIO_Real             volume;
    polygons_struct  clipped;
    File_formats     format;
    object_struct    **object_list;
    static  VIO_Vector   plane_normal = { 0.34118703, -0.06975647, 0.93740362 };
    static  VIO_Real     plane_constant = 23.435121182316;

    status = VIO_OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );

    status = input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list );

    if( status == VIO_OK )
        print( "Objects input.\n" );

    if( status == VIO_OK )
    {
        for_less( i, 0, n_objects )
        {
            if( status == VIO_OK && get_object_type(object_list[i]) )
            {
                clip_status = clip_polygons_to_plane(
                                  get_polygons_ptr(object_list[i]),
                                  &plane_normal, plane_constant,
                                  &clipped );

                volume = get_closed_polyhedron_volume( &clipped );
                print( "Clipped VIO_Volume: %g\n", volume );

                delete_polygons( get_polygons_ptr(object_list[i]) );
                *get_polygons_ptr(object_list[i]) = clipped;
            }
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
