#include  <module.h>

private  void  extract_isosurface(
    Minc_file         minc_file,
    Volume            volume,
    BOOLEAN           binary_flag,
    Real              min_threshold,
    Real              max_threshold,
    Real              valid_low,
    Real              valid_high,
    polygons_struct   *polygons );
private  void  extract_surface(
    BOOLEAN           binary_flag,
    Real              min_threshold,
    Real              max_threshold,
    Real              valid_low,
    Real              valid_high,
    int               x_size,
    int               y_size,
    float             ***slices,
    int               slice_index,
    int               ***point_ids[],
    polygons_struct   *polygons );

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_volume_filename, *output_filename;
    Volume               volume;
    Real                 min_threshold, max_threshold;
    Real                 valid_low, valid_high;
    Minc_file            minc_file;
    BOOLEAN              binary_flag;
    object_struct        *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  volume_file  object_file  min_thresh max_thresh\n",
               argv[0] );
        print( "       [valid_low  valid_high]\n" );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_threshold );
    (void) get_real_argument( min_threshold-1.0, &max_threshold );

    if( max_threshold < min_threshold )
    {
        binary_flag = FALSE;
        max_threshold = min_threshold;
    }
    else
        binary_flag = TRUE;

    (void) get_real_argument( 0.0, &valid_low );
    (void) get_real_argument( -1.0, &valid_high );

    volume = create_volume( 2, (char **) NULL, NC_UNSPECIFIED, FALSE,
                            0.0, 0.0 );

    minc_file = initialize_minc_input( input_volume_filename, volume,
                                       (minc_input_options *) NULL );

    if( minc_file == (Minc_file) NULL )
        return( 1 );

    object = create_object( POLYGONS );

    extract_isosurface( minc_file, volume, binary_flag,
                        min_threshold, max_threshold,
                        valid_low, valid_high, get_polygons_ptr(object) );

    (void) close_minc_input( minc_file );

    if( output_graphics_file( output_filename, BINARY_FORMAT, 1, &object ) !=OK)
        return( 1 );

    delete_object( object );

    return( 0 );
}

private  void  input_slice(
    Minc_file         minc_file,
    Volume            volume,
    float             **slice )
{
    int    x, y, sizes[MAX_DIMENSIONS], x_size, y_size;
    Real   amount_done;

    while( input_more_minc_file( minc_file, &amount_done ) )
    {}

    (void) advance_input_volume( minc_file );

    get_volume_sizes( volume, sizes );
    x_size = sizes[X];
    y_size = sizes[Y];

    for_less( x, 0, x_size )
    {
        for_less( y, 0, y_size )
        {
            GET_VALUE_2D( slice[x][y], volume, x, y );
        }
    }
}

private  void  clear_points(
    int         x_size,
    int         y_size,
    int         ***point_ids )
{
    int    x, y, edge;

    for_less( x, 0, x_size )
    {
        for_less( y, 0, y_size )
        {
            for_less( edge, 0, N_DIMENSIONS )
                point_ids[x][y][edge] = -1;
        }
    }
}

private  void  extract_isosurface(
    Minc_file         minc_file,
    Volume            volume,
    BOOLEAN           binary_flag,
    Real              min_threshold,
    Real              max_threshold,
    Real              valid_low,
    Real              valid_high,
    polygons_struct   *polygons )
{
    int             n_slices, sizes[MAX_DIMENSIONS], x_size, y_size, slice;
    int             ***point_ids[2], ***tmp_point_ids;
    float           **slices[2], **tmp_slices;
    progress_struct progress;

    n_slices = get_n_input_volumes( minc_file );

    get_volume_sizes( volume, sizes );
    x_size = sizes[X];
    y_size = sizes[Y];

    ALLOC2D( slices[0], x_size, y_size );
    ALLOC2D( slices[1], x_size, y_size );
    ALLOC3D( point_ids[0], x_size, y_size, N_DIMENSIONS );
    ALLOC3D( point_ids[1], x_size, y_size, N_DIMENSIONS );

    input_slice( minc_file, volume, slices[1] );

    clear_points( x_size, y_size, point_ids[0] );
    clear_points( x_size, y_size, point_ids[1] );

    initialize_polygons( polygons, WHITE, (Surfprop *) NULL );

    initialize_progress_report( &progress, FALSE, n_slices-1,
                                "Extracting Surface" );

    for_less( slice, 0, n_slices-1 )
    {
        tmp_slices = slices[0];
        slices[0] = slices[1];
        slices[1] = tmp_slices;
        input_slice( minc_file, volume, slices[1] );

        tmp_point_ids = point_ids[0];
        point_ids[0] = point_ids[1];
        point_ids[1] = tmp_point_ids;
        clear_points( x_size, y_size, point_ids[1] );

        extract_surface( binary_flag, min_threshold, max_threshold,
                         valid_low, valid_high,
                         x_size, y_size, slices, slice, point_ids, polygons );

        update_progress_report( &progress, slice+1 );
    }

    terminate_progress_report( &progress );

    if( polygons->n_points > 0 )
    {
        ALLOC( polygons->normals, polygons->n_points );
        compute_polygon_normals( polygons );
    }

    FREE2D( slices[0] );
    FREE2D( slices[1] );
    FREE3D( point_ids[0] );
    FREE3D( point_ids[1] );
}

private  int
                    point_index = get_point_index( &points[ind],
                                                   point_ids, polygons );

private  void  extract_surface(
    BOOLEAN           binary_flag,
    Real              min_threshold,
    Real              max_threshold,
    Real              valid_low,
    Real              valid_high,
    int               x_size,
    int               y_size,
    float             ***slices,
    int               slice_index,
    int               ***point_ids[],
    polygons_struct   *polygons )
{
    int                x, y, *sizes, tx, ty, tz, n_polys, ind;
    int                p, point_index, poly, size;
    voxel_point_type   *points;
    Real               corners[2][2][2];
    BOOLEAN            valid;

    for_less( x, 0, x_size-1 )
    {
        for_less( y, 0, y_size-1 )
        {
            valid = TRUE;
            for_less( tx, 0, 2 )
            for_less( ty, 0, 2 )
            for_less( tz, 0, 2 )
            {
                corners[tx][ty][tz] = slices[tz][x+tx][y+ty];
                if( valid_low <= valid_high &&
                    (corners[tx][ty][tz] < valid_low ||
                     corners[tx][ty][tz] > valid_high) )
                    valid = FALSE;
            }

            if( !valid )
                continue;

            n_polys = compute_isosurface_in_voxel( MARCHING_NO_HOLES,
                                  corners, binary_flag, min_threshold,
                                  max_threshold, &sizes, &points );

            ind = 0;
            for_less( poly, 0, n_polys )
            {
                size = sizes[poly];

                start_new_polygon( polygons );

                for_less( p, 0, size )
                {
                    point_index = get_point_index( &points[ind],
                                                   point_ids, polygons );

                    ADD_ELEMENT_TO_ARRAY( polygons->indices,
                             polygons->end_indices[polygons->n_items-1],
                             point_index, DEFAULT_CHUNK_SIZE );
                    
                    ++ind;
                }
            }
        }
    }
}
