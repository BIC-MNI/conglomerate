#include  <volume_io.h>
#include  <bicpl.h>

#define  NORMALIZE_EVERY 30

static  void  flatten_polygons(
    polygons_struct  *polygons,
    VIO_Real             step_ratio,
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR               src_filename, dest_filename;
    int                  n_objects, n_iters;
    VIO_File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;
    VIO_Real                 step_ratio;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj [step] [n_iters]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.2, &step_ratio );
    (void) get_int_argument( 1000, &n_iters );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != VIO_OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    flatten_polygons( polygons, step_ratio, n_iters );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

static  void  normalize_size(
    polygons_struct  *polygons,
    VIO_Point            *original_centroid,
    VIO_Real             original_size,
    int              n_neighbours[],
    int              *neighbours[] )
{
    int      point, neigh;
    VIO_Real     current_size, scale;
    VIO_Point    centroid;
    VIO_Vector   offset, diff;

    current_size = 0.0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( neigh, 0, n_neighbours[point] )
            current_size += sq_distance_between_points(
                              &polygons->points[point],
                              &polygons->points[neighbours[point][neigh]] );
    }

    scale = sqrt( original_size / current_size );

    get_points_centroid( polygons->n_points, polygons->points, &centroid );
    
    SUB_POINTS( offset, centroid, *original_centroid );

    for_less( point, 0, polygons->n_points )
    {
        SUB_POINTS( diff, polygons->points[point], centroid );
        SCALE_VECTOR( diff, diff, scale );
        ADD_POINT_VECTOR( polygons->points[point], *original_centroid, diff );
    }
}

static  VIO_Real  get_points_rms(
    int        n_points,
    VIO_Point      points1[],
    VIO_Point      points2[] )
{
    int      point;
    VIO_Vector   diff;
    float    movement;

    movement = 0.0f;
    for_less( point, 0, n_points )
    {
        SUB_POINTS( diff, points1[point], points2[point] );
        movement += Vector_x(diff) * Vector_x(diff) +
                    Vector_y(diff) * Vector_y(diff) +
                    Vector_z(diff) * Vector_z(diff);
    }

    return( sqrt( (VIO_Real) movement / (VIO_Real) n_points ) );
}

static  void  flatten(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              *neighbours[],
    VIO_Point            buffer[],
    VIO_Real             step_size )
{
    int      point, neigh, n, n_neighs;
    float    cx, cy, cz, fstep, a, b;

    fstep = (float) step_size;

    for_less( point, 0, polygons->n_points )
    {
        cx = 0.0f;
        cy = 0.0f;
        cz = 0.0f;
        n_neighs = n_neighbours[point];
        for_less( neigh, 0, n_neighs )
        {
            n = neighbours[point][neigh];
            cx += Point_x(polygons->points[n]);
            cy += Point_y(polygons->points[n]);
            cz += Point_z(polygons->points[n]);
        }

        a = 1.0f - fstep;
        b = fstep / (float) n_neighs;

        fill_Point( buffer[point],
                    a * Point_x(polygons->points[point]) + b  * cx,
                    a * Point_y(polygons->points[point]) + b  * cy,
                    a * Point_z(polygons->points[point]) + b  * cz );
    }

    for_less( point, 0, polygons->n_points )
        polygons->points[point] = buffer[point];
}

static  void  flatten_polygons(
    polygons_struct  *polygons,
    VIO_Real             step_ratio,
    int              n_iters )
{
    int    point, *n_neighbours, **neighbours, iter, neigh;
    VIO_Real   total_size, rms;
    VIO_Point  *buffer, centroid, *save_points;

    ALLOC( buffer, polygons->n_points );
    ALLOC( save_points, polygons->n_points );

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    get_points_centroid( polygons->n_points, polygons->points, &centroid );
    total_size = 0.0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( neigh, 0, n_neighbours[point] )
            total_size += sq_distance_between_points( &polygons->points[point],
                             &polygons->points[neighbours[point][neigh]] );
        save_points[point] = polygons->points[point];
    }
               
    for_less( iter, 0, n_iters )
    {
        flatten( polygons, n_neighbours, neighbours, buffer, step_ratio );

        if( ((iter+1) % NORMALIZE_EVERY) == 0 || iter == n_iters - 1 )
        {
            normalize_size( polygons, &centroid, total_size,
                            n_neighbours, neighbours );

            rms = get_points_rms( polygons->n_points, polygons->points,
                                  save_points );

            for_less( point, 0, polygons->n_points )
                save_points[point] = polygons->points[point];
            print( "Iter %4d: %g\n", iter+1, rms );
        }
        else
            print( "Iter %4d\n", iter+1 );

    }
   
    FREE( buffer );
    FREE( save_points );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     NULL, NULL );
}
