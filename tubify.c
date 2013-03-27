#include  <module.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    VIO_Real                 radius, smoothing_distance;
    File_formats         format;
    int                  i, q, n_objects, n_around;
    int                  n_new_objects, n_quadmeshes;
    quadmesh_struct      *quadmeshes;
    object_struct        **objects, **new_objects, *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  input_file  output_file  [n_around] [radius]\n",
               argv[0] );
        print( "       [smoothing_distance]\n" );
        return( 1 );
    }

    (void) get_int_argument( 10, &n_around );
    (void) get_real_argument( 2.0, &radius );
    (void) get_real_argument( 4.0, &smoothing_distance );

    if( input_graphics_file( input_filename,
                             &format, &n_objects, &objects ) != VIO_OK )
        return( 1 );

    n_new_objects = 0;

    for_less( i, 0, n_objects )
    {
        if( get_object_type( objects[i] ) == LINES )
        {
            smooth_lines( get_lines_ptr(objects[i]), smoothing_distance );
            n_quadmeshes = convert_lines_to_tubes( get_lines_ptr(objects[i]),
                                                   n_around, radius,
                                                   &quadmeshes );

            for_less( q, 0, n_quadmeshes )
            {
                object = create_object( QUADMESH );
                *get_quadmesh_ptr(object) = quadmeshes[q];
                ADD_ELEMENT_TO_ARRAY( new_objects, n_new_objects, object, 10 );
            }

            if( n_quadmeshes > 0 )
                FREE( quadmeshes );
        }
    }

    if( output_graphics_file( output_filename, format, n_new_objects,
                              new_objects ) != VIO_OK )
        return( 1 );

    delete_object_list( n_objects, objects );
    delete_object_list( n_new_objects, new_objects );

    return( 0 );
}
