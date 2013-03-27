#include  <volume_io.h>
#include  <bicpl.h>

  VIO_Status  process_object(
    object_struct  *object );

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: transform_objects  input.obj  input.xfm  [output.obj]\n\
\n\
     Transforms the input objects by the input transform, placing output in\n\
     output.obj if specified, otherwise in input.obj.\n\n";

    print_error( usage_str, executable );
}

static  void  transform_object(
    VIO_General_transform      *transform,
    object_struct          *object )
{
    int    i, n_points;
    VIO_Point  *points;
    VIO_Real   x, y, z;

    n_points = get_object_points( object, &points );

    for_less( i, 0, n_points )
    {
        general_transform_point( transform,
                                 (VIO_Real) Point_x(points[i]),
                                 (VIO_Real) Point_y(points[i]),
                                 (VIO_Real) Point_z(points[i]), &x, &y, &z );

        fill_Point( points[i], x, y, z );
    }
}

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR              input_filename, output_filename, transform_filename;
    VIO_STR              dummy;
    int                 i, n_objects;
    VIO_BOOL             invert;
    VIO_General_transform   transform;
    VIO_File_formats        format;
    object_struct       **object_list;
    VIO_BOOL             is_left_handed;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &transform_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );
    invert = get_string_argument( "", &dummy );

    if( input_transform_file( transform_filename, &transform ) != VIO_OK )
        return( 1 );

    if( invert )
        invert_general_transform( &transform );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK )
        return( 1 );

    if( get_transform_type( &transform ) == LINEAR )
    {
        is_left_handed = !is_transform_right_handed(
                             get_linear_transform_ptr(&transform) );
    }
    else
        is_left_handed = FALSE;

    for_less( i, 0, n_objects )
    {
        transform_object( &transform, object_list[i] );
        switch( get_object_type(object_list[i]) )
        {
        case POLYGONS:
            if( is_left_handed )
                reverse_polygons_vertices( get_polygons_ptr(object_list[i]) );
            compute_polygon_normals( get_polygons_ptr(object_list[i]) );
            break;

        case QUADMESH:
            if( is_left_handed )
                reverse_quadmesh_vertices( get_quadmesh_ptr(object_list[i]) );
            compute_quadmesh_normals( get_quadmesh_ptr(object_list[i]) );
            break;

        default:
            break;
        }
    }

    (void) output_graphics_file( output_filename, format,
                                 n_objects, object_list );

    delete_object_list( n_objects, object_list );

    return( 0 );
}
