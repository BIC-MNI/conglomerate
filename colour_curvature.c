#include  <def_mni.h>
#include  <def_module.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status         status;
    Real           max_curvature;
    BOOLEAN        set_to_zero_flag;
    char           *src_filename, *dest_filename;
    int            i, n_src_objects, n_dest_objects;
    File_formats   format;
    object_struct  **src_object_list, **dest_object_list;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &dest_filename ) )
    {
        (void) fprintf( stderr, "Must have two filename arguments.\n" );
        return( 1 );
    }

    (void) get_real_argument( 0.2, &max_curvature );
    (void) get_int_argument( 0, (int *) (&set_to_zero_flag) );

    status = input_graphics_file( src_filename, &format, &n_src_objects,
                                  &src_object_list );

    if( status == OK )
        status = input_graphics_file( dest_filename, &format, &n_dest_objects,
                                      &dest_object_list );

    print( "%d Objects input.\n", n_src_objects );

    if( status == OK && n_src_objects != n_dest_objects )
    {
        print( "Different number of objects in the two files.\n" );
        status = ERROR;
    }

    if( status == OK )
    {
        for_less( i, 0, n_src_objects )
        {
            if( status == OK && src_object_list[i]->object_type == POLYGONS &&
                dest_object_list[i]->object_type == POLYGONS &&
                polygons_are_same_topology(
                        get_polygons_ptr(src_object_list[i]),
                        get_polygons_ptr(dest_object_list[i]) ) )
            {
                colour_polygons_by_curvature(
                        get_polygons_ptr(src_object_list[i]),
                        get_polygons_ptr(dest_object_list[i]),
                        max_curvature, set_to_zero_flag );
            }
            else
                print( "Objects don't match.\n" );
        }

        if( status == OK )
            print( "Objects processed.\n" );
    }

    if( status == OK )
        status = output_graphics_file( dest_filename, format,
                                       n_dest_objects, dest_object_list );

    if( status == OK )
    {
        delete_object_list( n_src_objects, src_object_list );

        delete_object_list( n_dest_objects, dest_object_list );
    }

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}
