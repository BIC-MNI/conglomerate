#include  <volume_io.h>
#include  <bicpl.h>

#define  NORMALIZE_EVERY 30
#define  N_COMBINES 2

private  void  flatten_polygons(
    polygons_struct  *polygons,
    VIO_Real             step_ratio,
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR               src_filename, dest_filename;
    int                  n_objects, n_iters;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;
    VIO_Real                 step_ratio;

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
                             &object_list ) != VIO_OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    flatten_polygons( polygons, step_ratio, n_iters );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

private  void  normalize_size(
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

private  VIO_Real  get_points_rms(
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

private  void  flatten(
    polygons_struct  *polygons,
    int              n_related_points[],
    int              *related_points[],
    float            *related_weights[],
    VIO_Point            buffer[] )
{
    int      point, neigh, n;
    float    cx, cy, cz, w;

    for_less( point, 0, polygons->n_points )
    {
        cx = 0.0f;
        cy = 0.0f;
        cz = 0.0f;
        for_less( neigh, 0, n_related_points[point] )
        {
            n = related_points[point][neigh];
            w = related_weights[point][neigh];
            cx += w * Point_x(polygons->points[n]);
            cy += w * Point_y(polygons->points[n]);
            cz += w * Point_z(polygons->points[n]);
        }

        fill_Point( buffer[point], cx, cy, cz );
    }

    for_less( point, 0, polygons->n_points )
        polygons->points[point] = buffer[point];
}

private  void  combine_weights(
    int               n_points,
    int               n_related_points[],
    int               *related_points[],
    float             *related_weights[] )
{
    int    p, p2, point, neigh, neigh2, i, *next_n_related_points;
    int    **next_related_points, new_n, *point_list, size;
    float  **next_related_weights, *weight_list, weight;

    ALLOC( next_n_related_points, n_points );
    ALLOC( next_related_points, n_points );
    ALLOC( next_related_weights, n_points );

    point_list = NULL;
    weight_list = NULL;
    size = 0;

    for_less( point, 0, n_points )
    {
        new_n = 0;

        for_less( p, 0, n_related_points[point] )
        {
            neigh = related_points[point][p];
            weight = related_weights[point][p];
            for_less( p2, 0, n_related_points[neigh] )
            {
                neigh2 = related_points[neigh][p2];
                for_less( i, 0, new_n )
                {
                    if( point_list[i] == neigh2 )
                        break;
                }
                if( i == new_n )
                {
                    if( new_n >= size )
                    {
                        SET_ARRAY_SIZE( point_list, size, new_n + 1,
                                        DEFAULT_CHUNK_SIZE );
                        SET_ARRAY_SIZE( weight_list, size, new_n + 1,
                                        DEFAULT_CHUNK_SIZE );
                        size = new_n + 1;
                    }

                    point_list[new_n] = neigh2;
                    weight_list[new_n] = 0.0f;
                    ++new_n;
                }

                weight_list[i] += weight * related_weights[neigh][p2];
            }
        }

        next_n_related_points[point] = new_n;
        ALLOC( next_related_points[point], new_n );
        ALLOC( next_related_weights[point], new_n );

        for_less( i, 0, new_n )
        {
            next_related_points[point][i] = point_list[i];
            next_related_weights[point][i] = weight_list[i];
        }
    }

    for_less( point, 0, n_points )
    {
        n_related_points[point] = next_n_related_points[point];
        FREE( related_points[point] );
        FREE( related_weights[point] );
        related_points[point] = next_related_points[point];
        related_weights[point] = next_related_weights[point];
    }

    FREE( next_n_related_points );
}

private  void  create_weights(
    polygons_struct   *polygons,
    int               n_neighbours[],
    int               *neighbours[],
    VIO_Real              step_size,
    int               *n_related_points_ptr[],
    int               **related_points_ptr[],
    float             **related_weights_ptr[] )
{
    int    point, neigh, *n_related_points, combine;
    int    **related_points;
    float  **related_weights;

    ALLOC( n_related_points, polygons->n_points );
    ALLOC( related_points, polygons->n_points );
    ALLOC( related_weights, polygons->n_points );

    for_less( point, 0, polygons->n_points )
    {
        ALLOC( related_points[point], 1 + n_neighbours[point] );
        ALLOC( related_weights[point], 1 + n_neighbours[point] );

        n_related_points[point] = 1 + n_neighbours[point];
        related_points[point][0] = point;
        related_weights[point][0] = 1.0f - (float) step_size;
        for_less( neigh, 0, n_neighbours[point] )
        {
            related_points[point][neigh+1] = neighbours[point][neigh];
            related_weights[point][neigh+1] = (float) step_size /
                                            (float) n_neighbours[point];
        }
    }

    for_less( combine, 0, N_COMBINES )
    {
        print( "Combining: %d / %d\n", combine+1, N_COMBINES );
        combine_weights( polygons->n_points, n_related_points,
                         related_points, related_weights );
    }

    *n_related_points_ptr = n_related_points;
    *related_points_ptr = related_points;
    *related_weights_ptr = related_weights;
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    VIO_Real             step_ratio,
    int              n_iters )
{
    int    point, *n_neighbours, **neighbours, iter, neigh;
    VIO_Real   total_size, rms;
    VIO_Point  *buffer, centroid, *save_points;
    int    **related_points, *n_related_points;
    float  **related_weights;

    ALLOC( buffer, polygons->n_points );
    ALLOC( save_points, polygons->n_points );

    create_polygon_point_neighbours( polygons, &n_neighbours, &neighbours,
                                     NULL );

    get_points_centroid( polygons->n_points, polygons->points, &centroid );
    total_size = 0.0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( neigh, 0, n_neighbours[point] )
            total_size += sq_distance_between_points( &polygons->points[point],
                             &polygons->points[neighbours[point][neigh]] );
        save_points[point] = polygons->points[point];
    }

    create_weights( polygons, n_neighbours, neighbours, step_ratio,
                    &n_related_points, &related_points, &related_weights );
               
    for_less( iter, 0, n_iters )
    {
        flatten( polygons, n_related_points, related_points, related_weights,
                 buffer );

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

    for_less( point, 0, polygons->n_points )
    {
        FREE( related_points[point] );
        FREE( related_weights[point] );
    }
    FREE( n_related_points );
    FREE( related_points );
    FREE( related_weights );

    delete_polygon_point_neighbours( n_neighbours, neighbours, NULL );
}
