#include  <module.h>

private  void   intersect_planes_with_polygons(
    polygons_struct   *polygons,
    Point             *plane_origin,
    Vector            *plane_normal,
    lines_struct      *lines );

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *src_polygons_filename, *dest_lines_filename;
    char                 *axis_name;
    File_formats         format;
    int                  i, axis;
    Point                plane_origin;
    Vector               plane_normal;
    Real                 position;
    int                  n_objects, n_dest_objects;
    object_struct        **objects, **dest_objects, *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_polygons_filename ) ||
        !get_string_argument( "", &dest_lines_filename ) )
    {
        print( "Usage: %s  src_polygons  dest_lines x|y|z position\n", argv[0] );
        return( 1 );
    }

    (void) get_string_argument( "z", &axis_name );

    if( axis_name[0] >= 'x' && axis_name[0] <= 'z' )
        axis = (int) (axis_name[0] - 'x');
    else if( axis_name[0] >= 'X' && axis_name[0] <= 'Z' )
        axis = (int) (axis_name[0] - 'X');
    else
    {
        print( "Invalid axis specified.\n" );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &position );

    fill_Point( plane_origin, 0.0, 0.0, 0.0 );
    fill_Vector( plane_normal, 0.0, 0.0, 0.0 );

    Point_coord( plane_origin, axis ) = position;
    Vector_coord( plane_normal, axis ) = 1.0;

    if( input_graphics_file( src_polygons_filename,
                             &format, &n_objects, &objects ) != OK )
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

            add_object_to_list( &n_dest_objects, &dest_objects, object );
        }
    }

    if( output_graphics_file( dest_lines_filename, format, n_dest_objects,
                              dest_objects ) != OK )
        return( 1 );

    delete_object_list( n_objects, objects );
    delete_object_list( n_dest_objects, dest_objects );

    return( 0 );
}

#define  MAX_POINTS  1000

private  void   intersect_planes_with_polygons(
    polygons_struct   *polygons,
    Point             *plane_origin,
    Vector            *plane_normal,
    lines_struct      *lines )
{
    int     n_points;
    int     poly, edge, size;
    int     point_index1, point_index2;
    Point   points[2], int_point;

    initialize_lines( lines, WHITE );

    for_less( poly, 0, polygons->n_items )
    {
        n_points = 0;

        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( edge, 0, size )
        {
            point_index1 = polygons->indices[
                     POINT_INDEX(polygons->end_indices,poly,edge)];
            point_index2 = polygons->indices[
                     POINT_INDEX(polygons->end_indices,poly,(edge+1)%size)];

            if( line_segment_intersects_plane( &polygons->points[point_index1],
                                               &polygons->points[point_index2],
                                               plane_origin, plane_normal,
                                               &int_point ) )
            {
                if( n_points == 2 )
                {
                    n_points = 0;
                    break;
                }

                points[n_points] = int_point;
                ++n_points;
            }
        }

        if( n_points == 2 )
        {
            start_new_line( lines );
            add_point_to_line( lines, &points[0] );
            add_point_to_line( lines, &points[1] );
        }
    }
}
