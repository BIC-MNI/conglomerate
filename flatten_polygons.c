#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  NORMALIZE_EVERY 30

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Real             step_ratio,
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename;
    int                  n_objects, n_iters;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;
    Real                 step_ratio;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj [n]\n", argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.2, &step_ratio );
    (void) get_int_argument( 1000, &n_iters );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    flatten_polygons( polygons, step_ratio, n_iters );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

private  void  normalize_size(
    polygons_struct  *polygons,
    Point            *original_centroid,
    Real             original_size,
    int              n_neighbours[],
    int              *neighbours[] )
{
    int      point, neigh;
    Real     current_size, scale;
    Point    centroid;
    Vector   offset, diff;

    current_size = 0.0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( neigh, 0, n_neighbours[point] )
            current_size += distance_between_points( &polygons->points[point],
                             &polygons->points[neighbours[point][neigh]] );
    }

    scale = original_size / current_size;

    get_points_centroid( polygons->n_points, polygons->points, &centroid );
    
    SUB_POINTS( offset, centroid, *original_centroid );

    for_less( point, 0, polygons->n_points )
    {
        SUB_POINTS( diff, polygons->points[point], centroid );
        SCALE_VECTOR( diff, diff, scale );
        ADD_POINT_VECTOR( polygons->points[point], *original_centroid, diff );
    }
}

private  Real  flatten(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              *neighbours[],
    Point            buffer[],
    Real             step_size )
{
    int      point, neigh, n;
    Vector   diff;
    Real     cx, cy, cz, movement;

    for_less( point, 0, polygons->n_points )
    {
        cx = 0.0;
        cy = 0.0;
        cz = 0.0;
        for_less( neigh, 0, n_neighbours[point] )
        {
            n = neighbours[point][neigh];
            cx += RPoint_x(polygons->points[n]);
            cy += RPoint_y(polygons->points[n]);
            cz += RPoint_z(polygons->points[n]);
        }

        fill_Point( buffer[point],
                    (1.0 - step_size) * RPoint_x(polygons->points[point]) +
                           step_size  * cx / (Real) n_neighbours[point],
                    (1.0 - step_size) * RPoint_y(polygons->points[point]) +
                           step_size  * cy / (Real) n_neighbours[point],
                    (1.0 - step_size) * RPoint_z(polygons->points[point]) +
                           step_size  * cz / (Real) n_neighbours[point] );
    }

    movement = 0.0;
    for_less( point, 0, polygons->n_points )
    {
        SUB_POINTS( diff, buffer[point], polygons->points[point] );
        movement += DOT_VECTORS( diff, diff );
        polygons->points[point] = buffer[point];
    }

    return( sqrt( movement / (Real) polygons->n_points ) );
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Real             step_ratio,
    int              n_iters )
{
    int    point, *n_neighbours, **neighbours, iter, neigh;
    Real   total_size, movement;
    Point  *buffer, centroid;

    ALLOC( buffer, polygons->n_points );

    create_polygon_point_neighbours( polygons, &n_neighbours, &neighbours,
                                     NULL );

    get_points_centroid( polygons->n_points, polygons->points, &centroid );
    total_size = 0.0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( neigh, 0, n_neighbours[point] )
            total_size += distance_between_points( &polygons->points[point],
                             &polygons->points[neighbours[point][neigh]] );
    }
               
    for_less( iter, 0, n_iters )
    {
        movement = flatten( polygons,
                            n_neighbours, neighbours, buffer, step_ratio );

        if( ((iter+1) % NORMALIZE_EVERY) == 0 || iter == n_iters - 1 )
        {
            normalize_size( polygons, &centroid, total_size,
                            n_neighbours, neighbours );
        }

        print( "Iter %4d: %g\n", iter+1, movement );

    }
   
    FREE( buffer );

    delete_polygon_point_neighbours( n_neighbours, neighbours, NULL );
}
