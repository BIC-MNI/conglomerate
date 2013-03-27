#include  <volume_io.h>
#include  <bicpl.h>

#define  TOLERANCE  1.0e-3

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_Real           x, y, z, n_points, plane_constant, dist;
    VIO_Point          position, points[2000];
    VIO_Vector         normal;
    VIO_Status         status;
    char           *input_filename;
    int            i, n_objects;
    File_formats   format;
    object_struct  **object_list;
    polygons_struct  *polygons;

    status = VIO_OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_real_argument( 0.0, &x ) ||
        !get_real_argument( 0.0, &y ) ||
        !get_real_argument( 0.0, &z ) )
    {
        print_error( "Must have a filename argument.\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list ) != VIO_OK ||
        n_objects != 1 || get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "Must have a polygon in the file.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    fill_Point( position, x, y, z );

    for_less( i, 0, polygons->n_items )
    {
        n_points = get_polygon_points( polygons, i, points );
        get_plane_through_points( n_points, points, &normal, &plane_constant );
        dist = distance_from_plane( &position, &normal, plane_constant );
        if( dist >= -TOLERANCE && dist <= TOLERANCE &&
            point_within_polygon( &position, n_points, points, &normal ) )
            print( "%d\n", i );
    }

    return( VIO_OK );
}
