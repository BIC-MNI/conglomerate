#include  <bicpl.h>

#define  BINTREE_FACTOR  0.5

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               src_polygons_filename, dest_polygons_filename;
    STRING               input_filename, output_filename;
    File_formats         format;
    int                  i, n_points;
    int                  n_src_objects, n_dest_objects, n_objects;
    Point                *points;
    object_struct        **objects, **src_objects, **dest_objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_polygons_filename ) ||
        !get_string_argument( "", &dest_polygons_filename ) ||
        !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  src_polygons  dest_polygons input_file output_file\n",
               argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_polygons_filename,
                             &format, &n_src_objects, &src_objects ) != OK )
        return( 1 );

    if( input_graphics_file( dest_polygons_filename,
                             &format, &n_dest_objects, &dest_objects ) != OK )
        return( 1 );

    if( n_src_objects != 1 || get_object_type( src_objects[0] ) != POLYGONS ||
        n_dest_objects != 1 || get_object_type( dest_objects[0] ) != POLYGONS )
    {
        print( "Must specify polygons files.\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    create_polygons_bintree( get_polygons_ptr(src_objects[0]),
                             ROUND( (Real) get_polygons_ptr(
                                            src_objects[0])->n_items *
                                    BINTREE_FACTOR ) );

    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( objects[i], &points );

        polygon_transform_points( get_polygons_ptr(src_objects[0]),
                                  get_polygons_ptr(dest_objects[0]),
                                  n_points, points, points );
    }

    if( output_graphics_file( output_filename, format, n_objects,
                              objects ) != OK )
        return( 1 );

    delete_object_list( n_src_objects, src_objects );
    delete_object_list( n_dest_objects, dest_objects );
    delete_object_list( n_objects, objects );

    return( 0 );
}
