#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, output_filename;
    int              i, n_objects;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons;
    Point            *original_points, centroid;
    Vector           offset;
    Real             scaling;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &scaling ) )
    {
        print_error(
          "Usage: %s input.obj output.obj scale\n",
          argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK ||
        n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
    {
        print( "File %s must contain exactly 1 surface.\n", input_filename );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    original_points = polygons->points;
    get_points_centroid( polygons->n_points, original_points, &centroid );

    ALLOC( polygons->points, polygons->n_points );

    for_less( i, 0, polygons->n_points )
    {
        SUB_POINTS( offset, original_points[i], centroid );
        SCALE_VECTOR( offset, offset, scaling );
        ADD_POINT_VECTOR( polygons->points[i], centroid, offset );
    }

    compute_polygon_normals( polygons );

    (void) output_graphics_file( output_filename, format,
                                 n_objects, object_list );

    return( 0 );
}
