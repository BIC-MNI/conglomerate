#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status           status;
    STRING           filename, output_filename;
    int              i, n_objects;
    int              n_sets, n_points, n_set_points;
    object_struct    *out_object;
    object_struct    **object_list;
    Point            *points, *set_points;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
          "Usage: %s output.obj\n", argv[0] );
        print_error( "         [input1.obj] [input2.obj] ...\n" );
        return( 1 );
    }

    n_sets = 0;

    while( get_string_argument( NULL, &filename ) )
    {
        if( input_objects_any_format( NULL, filename,
                                      GREEN, 1.0, SPHERE_MARKER,
                                      &n_objects, &object_list ) != OK )

        n_points = get_object_points( object_list[0], &points );

        if( n_sets == 0 )
        {
            out_object = object_list[0];
            n_set_points = n_points;
            set_points = points;
        }
        else
        {
            if( n_points != n_set_points )
            {
                print( "N points mismatch\n" );
                return( 1 );
            }

            for_less( i, 0, n_points )
            {
                ADD_POINTS( set_points[i], set_points[i], points[i] );
            }
        }

        print( "%d:  %s\n", n_sets, filename );

        if( n_sets > 0 )
            delete_object_list( n_objects, object_list );

        ++n_sets;
    }

    if( get_object_type( out_object ) == POLYGONS )
        compute_polygon_normals( get_polygons_ptr(out_object) );

    if( status == OK )
        status = output_graphics_file( output_filename, BINARY_FORMAT,
                                       1, &out_object );

    return( status != OK );
}
