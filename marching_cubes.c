#include  <volume_io.h>
#include  <bicpl/marching.h>

#define  CHUNK_SIZE   1000000

static  void  extract_isosurface(
    Minc_file         minc_file,
    VIO_Volume            volume,
    Minc_file         label_file,
    VIO_Volume            label_volume,
    VIO_Real              min_label,
    VIO_Real              max_label,
    int               spatial_axes[],
    VIO_General_transform *voxel_to_world_transform,
    Marching_cubes_methods  method,
    VIO_BOOL           binary_flag,
    VIO_Real              min_threshold,
    VIO_Real              max_threshold,
    VIO_Real              valid_low,
    VIO_Real              valid_high,
    polygons_struct   *polygons );
static  void  extract_surface(
    Marching_cubes_methods  method,
    VIO_BOOL           binary_flag,
    VIO_Real              min_threshold,
    VIO_Real              max_threshold,
    VIO_Real              valid_low,
    VIO_Real              valid_high,
    int               x_size,
    int               y_size,
    VIO_Real              ***slices,
    VIO_Real              min_label,
    VIO_Real              max_label,
    VIO_Real              ***label_slices,
    int               slice_index,
    VIO_BOOL           right_handed,
    int               spatial_axes[],
    VIO_General_transform *voxel_to_world_transform,
    int               ***point_ids[],
    polygons_struct   *polygons );

static  VIO_STR    dimension_names_3D[] = { MIzspace, MIyspace, MIxspace };
static  VIO_STR    dimension_names[] = { MIyspace, MIxspace };

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: marching_cubes  input.mnc  output.obj  threshold\n\
       marching_cubes  input.mnc  output.obj  min_threshold max_threshold\n\
\n\
     Creates a polygonal surface of either the thresholded volume, or the\n\
     boundary of the region of values between min and max threshold.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               input_volume_filename, output_filename;
    VIO_STR               label_filename;
    VIO_Volume               volume, label_volume;
    VIO_Real                 min_threshold, max_threshold;
    VIO_Real                 min_label, max_label;
    VIO_Real                 valid_low, valid_high;
    Minc_file            minc_file, label_file;
    VIO_BOOL              binary_flag;
    int                  c, spatial_axes[VIO_N_DIMENSIONS];
    int                  int_method;
    Marching_cubes_methods  method;
    object_struct        *object;
    volume_input_struct  volume_input;
    VIO_General_transform    voxel_to_world_transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_threshold );
    (void) get_real_argument( min_threshold-1.0, &max_threshold );
    (void) get_int_argument( (int) MARCHING_NO_HOLES,  &int_method );
    method = (Marching_cubes_methods) int_method;

    if( max_threshold < min_threshold )
    {
        binary_flag = FALSE;
        max_threshold = min_threshold;
    }
    else
        binary_flag = TRUE;

    (void) get_real_argument( 0.0, &valid_low );
    (void) get_real_argument( -1.0, &valid_high );

    if( get_string_argument( NULL, &label_filename ) )
    {
        if( !get_real_argument( 0.0, &min_label ) ||
            !get_real_argument( 0.0, &max_label ) )
        {
            print( "Missing label range.\n" );
            return( 1 );
        }
    }
    else
    {
        min_label = 0.0;
        max_label = -1.0;
    }

    if( start_volume_input( input_volume_filename, 3, dimension_names_3D,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != VIO_OK )
        return( 0 );


    copy_general_transform( &volume_input.minc_file->voxel_to_world_transform,
                            &voxel_to_world_transform );

    for_less( c, 0, VIO_N_DIMENSIONS )
        spatial_axes[c] = volume->spatial_axes[c];

    delete_volume_input( &volume_input );
    delete_volume( volume );

    volume = create_volume( 2, dimension_names, NC_UNSPECIFIED, FALSE,
                            0.0, 0.0 );

    minc_file = initialize_minc_input( input_volume_filename, volume,
                                       (minc_input_options *) NULL );

    if( minc_file == (Minc_file) NULL )
        return( 1 );

    if( min_label <= max_label )
    {
        label_volume = create_volume( 2, volume->dimension_names,
                                      NC_UNSPECIFIED, FALSE, 0.0, 0.0 );

        label_file = initialize_minc_input( label_filename, label_volume,
                                            (minc_input_options *) NULL );

        if( label_file == (Minc_file) NULL )
            return( 1 );
    }
    else
    {
        label_volume = NULL;
        label_file = NULL;
    }


    object = create_object( POLYGONS );

    extract_isosurface( minc_file, volume,
                        label_file, label_volume, min_label, max_label,
                        spatial_axes,
                        &voxel_to_world_transform,
                        method, binary_flag,
                        min_threshold, max_threshold,
                        valid_low, valid_high, get_polygons_ptr(object) );

    (void) close_minc_input( minc_file );

    if( min_label <= max_label )
    {
        (void) close_minc_input( label_file );
        delete_volume( label_volume );
    }

    if( output_graphics_file( output_filename, BINARY_FORMAT, 1, &object ) !=VIO_OK)
        return( 1 );

    delete_object( object );

    delete_volume( volume );

    delete_marching_cubes_table();

    delete_general_transform( &voxel_to_world_transform );

    return( 0 );
}

static  void  clear_slice(
    VIO_Volume            volume,
    VIO_Real              **slice )
{
    int    x, y, sizes[VIO_MAX_DIMENSIONS];

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[0] )
    for_less( y, 0, sizes[1] )
        slice[x][y] = 0.0;
}

static  void  input_slice(
    Minc_file         minc_file,
    VIO_Volume            volume,
    VIO_Real              **slice )
{
    int    sizes[VIO_MAX_DIMENSIONS];
    VIO_Real   amount_done;

    while( input_more_minc_file( minc_file, &amount_done ) )
    {}

    (void) advance_input_volume( minc_file );

    get_volume_sizes( volume, sizes );

    get_volume_value_hyperslab_2d( volume, 0, 0, sizes[0], sizes[1],
                                   &slice[0][0] );
}

static  VIO_Real  get_slice_value(
    VIO_Real      ***slices,
    int       x_size,
    int       y_size,
    int       z,
    int       x,
    int       y )
{
    if( x < 0 || x >= x_size || y < 0 || y >= y_size )
        return( 0.0 );
    else
        return( slices[z][x][y] );
}

static  void  clear_points(
    int         x_size,
    int         y_size,
    int         max_edges,
    int         ***point_ids )
{
    int    x, y, edge;

    for_less( x, 0, x_size+2 )
    {
        for_less( y, 0, y_size+2 )
        {
            for_less( edge, 0, max_edges )
                point_ids[x][y][edge] = -1;
        }
    }
}

static  void   get_world_point(
    VIO_Real                slice,
    VIO_Real                x,
    VIO_Real                y,
    int                 spatial_axes[],
    VIO_General_transform   *voxel_to_world_transform,
    VIO_Point               *point )
{
    int            c;
    VIO_Real           xw, yw, zw;
    VIO_Real           real_voxel[VIO_N_DIMENSIONS], voxel_pos[VIO_N_DIMENSIONS];

    real_voxel[0] = slice;
    real_voxel[1] = x;
    real_voxel[2] = y;

    for_less( c, 0, VIO_N_DIMENSIONS )
    {
        if( spatial_axes[c] >= 0 )
            voxel_pos[c] = real_voxel[spatial_axes[c]];
        else
            voxel_pos[c] = 0.0;
    }

    general_transform_point( voxel_to_world_transform,
                             voxel_pos[VIO_X], voxel_pos[VIO_Y], voxel_pos[VIO_Z],
                             &xw, &yw, &zw );

    fill_Point( *point, xw, yw, zw );
}

static  void  extract_isosurface(
    Minc_file         minc_file,
    VIO_Volume            volume,
    Minc_file         label_file,
    VIO_Volume            label_volume,
    VIO_Real              min_label,
    VIO_Real              max_label,
    int               spatial_axes[],
    VIO_General_transform *voxel_to_world_transform,
    Marching_cubes_methods  method,
    VIO_BOOL           binary_flag,
    VIO_Real              min_threshold,
    VIO_Real              max_threshold,
    VIO_Real              valid_low,
    VIO_Real              valid_high,
    polygons_struct   *polygons )
{
    int             n_slices, sizes[VIO_MAX_DIMENSIONS], x_size, y_size, slice;
    int             ***point_ids[2], ***tmp_point_ids;
    int             max_edges;
    VIO_Real            **slices[2], **tmp_slices;
    VIO_Real            **label_slices[2];
    VIO_progress_struct progress;
    VIO_Surfprop        spr;
    VIO_Point           point000, point100, point010, point001;
    VIO_Vector          v100, v010, v001, perp;
    VIO_BOOL         right_handed;

    get_world_point( 0.0, 0.0, 0.0, spatial_axes, voxel_to_world_transform,
                     &point000 );
    get_world_point( 1.0, 0.0, 0.0, spatial_axes, voxel_to_world_transform,
                     &point100 );
    get_world_point( 0.0, 1.0, 0.0, spatial_axes, voxel_to_world_transform,
                     &point010 );
    get_world_point( 0.0, 0.0, 1.0, spatial_axes, voxel_to_world_transform,
                     &point001 );

    SUB_POINTS( v100, point100, point000 );
    SUB_POINTS( v010, point010, point000 );
    SUB_POINTS( v001, point001, point000 );
    CROSS_VECTORS( perp, v100, v010 );

    right_handed = DOT_VECTORS( perp, v001 ) >= 0.0;

    n_slices = get_n_input_volumes( minc_file );

    get_volume_sizes( volume, sizes );
    x_size = sizes[VIO_X];
    y_size = sizes[VIO_Y];

    VIO_ALLOC2D( slices[0], x_size, y_size );
    VIO_ALLOC2D( slices[1], x_size, y_size );

    if( label_volume != NULL )
    {
        VIO_ALLOC2D( label_slices[0], x_size, y_size );
        VIO_ALLOC2D( label_slices[1], x_size, y_size );
    }

    max_edges = get_max_marching_edges( method );

    VIO_ALLOC3D( point_ids[0], x_size+2, y_size+2, max_edges );
    VIO_ALLOC3D( point_ids[1], x_size+2, y_size+2, max_edges );

    clear_slice( volume, slices[1] );
    if( label_volume != NULL )
        clear_slice( volume, label_slices[1] );

    clear_points( x_size, y_size, max_edges, point_ids[0] );
    clear_points( x_size, y_size, max_edges, point_ids[1] );

    Surfprop_a(spr) = 0.3f;
    Surfprop_d(spr) = 0.6f;
    Surfprop_s(spr) = 0.6f;
    Surfprop_se(spr) = 30.0f;
    Surfprop_t(spr) = 1.0f;
    initialize_polygons( polygons, WHITE, &spr );

    initialize_progress_report( &progress, FALSE, n_slices+1,
                                "Extracting Surface" );

    for_less( slice, -1, n_slices )
    {
        tmp_slices = slices[0];
        slices[0] = slices[1];
        slices[1] = tmp_slices;
        if( slice < n_slices - 1 )
            input_slice( minc_file, volume, slices[1] );
        else
            clear_slice( volume, slices[1] );

        if( label_volume != NULL )
        {
            tmp_slices = label_slices[0];
            label_slices[0] = label_slices[1];
            label_slices[1] = tmp_slices;
            if( slice < n_slices - 1 )
                input_slice( label_file, label_volume, label_slices[1] );
            else
                clear_slice( volume, label_slices[1] );
        }

        tmp_point_ids = point_ids[0];
        point_ids[0] = point_ids[1];
        point_ids[1] = tmp_point_ids;
        clear_points( x_size, y_size, max_edges, point_ids[1] );

        extract_surface( method, binary_flag, min_threshold, max_threshold,
                         valid_low, valid_high,
                         x_size, y_size, slices,
                         min_label, max_label, label_slices, slice,
                         right_handed, spatial_axes, voxel_to_world_transform,
                         point_ids, polygons );

        update_progress_report( &progress, slice+2 );
    }

    terminate_progress_report( &progress );

    if( polygons->n_points > 0 )
    {
        ALLOC( polygons->normals, polygons->n_points );
        compute_polygon_normals( polygons );
    }

    VIO_FREE2D( slices[0] );
    VIO_FREE2D( slices[1] );

    if( label_volume != NULL )
    {
        VIO_FREE2D( label_slices[0] );
        VIO_FREE2D( label_slices[1] );
    }

    VIO_FREE3D( point_ids[0] );
    VIO_FREE3D( point_ids[1] );
}

static  int   get_point_index(
    int                 x,
    int                 y,
    int                 slice_index,
    int                 x_size,
    int                 y_size,
    voxel_point_type    *point,
    VIO_Real                corners[2][2][2],
    int                 spatial_axes[],
    VIO_General_transform   *voxel_to_world_transform,
    VIO_BOOL             binary_flag,
    VIO_Real                min_threshold,
    VIO_Real                max_threshold,
    int                 ***point_ids[],
    polygons_struct     *polygons )
{
    int            voxel[VIO_N_DIMENSIONS], edge, point_index;
    int            edge_voxel[VIO_N_DIMENSIONS];
    VIO_Real           v[VIO_N_DIMENSIONS];
    VIO_Point          world_point;
    Point_classes  point_class;

    voxel[VIO_X] = x + point->coord[VIO_X];
    voxel[VIO_Y] = y + point->coord[VIO_Y];
    voxel[VIO_Z] = point->coord[VIO_Z];
    edge = point->edge_intersected;

    point_index = point_ids[voxel[VIO_Z]][voxel[VIO_X]+1][voxel[VIO_Y]+1][edge];
    if( point_index < 0 )
    {
        edge_voxel[VIO_X] = point->coord[VIO_X];
        edge_voxel[VIO_Y] = point->coord[VIO_Y];
        edge_voxel[VIO_Z] = point->coord[VIO_Z];
        point_class = get_isosurface_point( corners, edge_voxel, edge,
                                            binary_flag,
                                            min_threshold, max_threshold, v );

        get_world_point( v[VIO_Z] + (VIO_Real) slice_index,
                         v[VIO_X] + (VIO_Real) x, v[VIO_Y] + (VIO_Real) y,
                         spatial_axes, voxel_to_world_transform, &world_point );

        point_index = polygons->n_points;
        ADD_ELEMENT_TO_ARRAY( polygons->points, polygons->n_points,
                              world_point, CHUNK_SIZE );

        point_ids[voxel[VIO_Z]][voxel[VIO_X]+1][voxel[VIO_Y]+1][edge] = point_index;
    }

    return( point_index );
}

static  void  extract_surface(
    Marching_cubes_methods  method,
    VIO_BOOL           binary_flag,
    VIO_Real              min_threshold,
    VIO_Real              max_threshold,
    VIO_Real              valid_low,
    VIO_Real              valid_high,
    int               x_size,
    int               y_size,
    VIO_Real              ***slices,
    VIO_Real              min_label,
    VIO_Real              max_label,
    VIO_Real              ***label_slices,
    int               slice_index,
    VIO_BOOL           right_handed,
    int               spatial_axes[],
    VIO_General_transform *voxel_to_world_transform,
    int               ***point_ids[],
    polygons_struct   *polygons )
{
    int                x, y, *sizes, tx, ty, tz, n_polys, ind;
    int                p, point_index, poly, size, start_points, dir;
    voxel_point_type   *points;
    VIO_Real               corners[2][2][2], label;
    VIO_BOOL            valid;

    for_less( x, -1, x_size )
    {
        for_less( y, -1, y_size )
        {
            valid = TRUE;
            for_less( tx, 0, 2 )
            for_less( ty, 0, 2 )
            for_less( tz, 0, 2 )
            {
                corners[tx][ty][tz] = get_slice_value( slices, x_size, y_size,
                                                       tz, x + tx, y + ty );

                if( valid_low <= valid_high &&
                    (corners[tx][ty][tz] < min_threshold ||
                    corners[tx][ty][tz] > max_threshold) &&
                    (corners[tx][ty][tz] < valid_low ||
                     corners[tx][ty][tz] > valid_high) )
                    valid = FALSE;

                if( min_label <= max_label )
                {
                    label = get_slice_value( label_slices, x_size, y_size,
                                             tz, x + tx, y + ty );
                    if( label < min_label || label > max_label )
                        corners[tx][ty][tz] = 0.0;
                }
            }

            if( !valid )
                continue;

            n_polys = compute_isosurface_in_voxel( method, x, y, slice_index,
                                  corners, binary_flag, min_threshold,
                                  max_threshold, &sizes, &points );

            if( n_polys == 0 )
                continue;

#ifdef DEBUG
{
int   ind, i;
print( "Okay %d %d: \n", x, y );
ind = 0;
for_less( poly, 0, n_polys )
{
    for_less( i, 0, sizes[poly] )
    {
        print( "%d %d %d : %d\n", points[ind].coord[0],
               points[ind].coord[1],
               points[ind].coord[2],
               points[ind].edge_intersected );
        ++ind;
    }

    print( "\n" );
}
}
#endif

            if( right_handed )
            {
                start_points = 0;
                dir = 1;
            }
            else
            {
                start_points = sizes[0]-1;
                dir = -1;
            }

            for_less( poly, 0, n_polys )
            {
                size = sizes[poly];

                start_new_polygon( polygons );

                /*--- orient polygons properly */

                for_less( p, 0, size )
                {
                    ind = start_points + p * dir;
                    point_index = get_point_index( x, y, slice_index,
                                   x_size, y_size, &points[ind], corners,
                                   spatial_axes, voxel_to_world_transform,
                                   binary_flag, min_threshold, max_threshold,
                                   point_ids, polygons );

                    ADD_ELEMENT_TO_ARRAY( polygons->indices,
                             polygons->end_indices[polygons->n_items-1],
                             point_index, CHUNK_SIZE );
                }

                if( right_handed )
                    start_points += size;
                else if( poly < n_polys-1 )
                    start_points += sizes[poly+1];
            }
        }
    }
}
