#include  <volume_io.h>
#include  <bicpl.h>

static  void  contour_volume_slice(
    VIO_Volume        volume,
    int           axis_index,
    VIO_Real          slice_pos,
    VIO_BOOL       world_flag,
    VIO_Real          contour_value,
    lines_struct  *lines );

static  int  get_axis_index(
    VIO_STR   name,
    int      *axis_index,
    VIO_BOOL  *world_flag )
{
    *axis_index = -1;
    *world_flag = FALSE;

    switch( name[0] )
    {
    case 'x':
    case 'X': *axis_index = VIO_X; break;
    case 'y':
    case 'Y': *axis_index = VIO_Y; break;
    case 'z':
    case 'Z': *axis_index = VIO_Z; break;
    }

    if( name[1] == 'w' || name[1] == 'W' )
        *world_flag = TRUE;

    return( *axis_index >= 0 );
}

static  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  volume_file  output_file  x|y|z|xw|yw|zw slice_pos\n\
       [contour_value1] [contour_value2] ... \n\
\n\
     Creates contour lines for the given slice at the given contour values.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               input_volume_filename, output_filename;
    VIO_STR               axis_name;
    VIO_Volume               volume;
    int                  n_objects, axis_index;
    object_struct        **objects, *object;
    VIO_Real                 contour_value, slice_pos;
    VIO_BOOL              world_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &axis_name ) ||
        !get_axis_index( axis_name, &axis_index, &world_flag ) ||
        !get_real_argument( 0.0, &slice_pos ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    n_objects = 0;

    while( get_real_argument( 0.0, &contour_value ) )
    {
        object = create_object( LINES );

        initialize_lines( get_lines_ptr(object), WHITE );

        contour_volume_slice( volume, axis_index, slice_pos, world_flag,
                              contour_value, get_lines_ptr(object) );

        add_object_to_list( &n_objects, &objects, object );
    }

    if( output_graphics_file( output_filename, ASCII_FORMAT, n_objects,
                              objects ) != VIO_OK )
        return( 1 );

    delete_object_list( n_objects, objects );
    delete_volume( volume );

    return( 0 );
}

typedef  struct
{
    int            ids[3];
    VIO_SCHAR   lower_left_fence;
} node_struct;

static  void  record_values(
    VIO_Volume        volume,
    int           a1,
    int           a2,
    int           axis_index,
    VIO_Real          slice_pos,
    int           x_size,
    int           y_size,
    VIO_Real          contour_value,
    VIO_Real          **values )
{
    int  x, y;
    VIO_Real voxel[VIO_N_DIMENSIONS], value;

    voxel[axis_index] = slice_pos;
    for_less( x, 0, x_size )
    {
        voxel[a1] = (VIO_Real) x;
        for_less( y, 0, y_size )
        {
            voxel[a2] = (VIO_Real) y;
            evaluate_volume( volume, voxel, FALSE, 0, FALSE, 0.0,
                             &value, NULL, NULL );
            values[x][y] = value - contour_value;
        }
    }
}

static  void  get_point(
    VIO_Volume   volume,
    int      axis_index,
    VIO_Real     slice_pos,
    VIO_Real     x,
    VIO_Real     y,
    VIO_Point    *point )
{
    VIO_Real   voxel[VIO_N_DIMENSIONS], xw, yw, zw;

    voxel[axis_index] = slice_pos;
    voxel[(axis_index+1) % VIO_N_DIMENSIONS] = x;
    voxel[(axis_index+2) % VIO_N_DIMENSIONS] = y;

    convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );

    fill_Point( *point, xw, yw, zw );
}

static  void  create_node_points(
    VIO_Volume       volume,
    int          axis_index,
    VIO_Real         slice_pos,
    int          x_size,
    int          y_size,
    VIO_Real         **values,
    node_struct  **node_info,
    int          *n_points,
    VIO_Point        *points[] )
{
    int    x, y;
    VIO_Real   this_value, neigh_value, fraction;
    VIO_Point  point;

    for_less( x, 0, x_size )
    {
        for_less( y, 0, y_size )
        {
            node_info[x][y].ids[0] = -1;
            node_info[x][y].ids[1] = -1;
            node_info[x][y].ids[2] = -1;
        }
    }

    for_less( x, 0, x_size-1 )
    {
        for_less( y, 0, y_size-1 )
        {
            this_value = values[x][y];
            neigh_value = values[x+1][y];

            if( this_value == 0.0 && neigh_value > 0.0 )
            {
                if( node_info[x][y].ids[2] < 0 )
                {
                    get_point( volume, axis_index, slice_pos,
                               (VIO_Real) x, (VIO_Real) y, &point );
                    ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                          DEFAULT_CHUNK_SIZE );
                    node_info[x][y].ids[2] = *n_points-1;
                }
                node_info[x][y].ids[0] = node_info[x][y].ids[2];
            }
            else if( this_value > 0.0 && neigh_value == 0.0 )
            {
                if( node_info[x+1][y].ids[2] < 0 )
                {
                    get_point( volume, axis_index, slice_pos,
                               (VIO_Real) x+1.0, (VIO_Real) y, &point );
                    ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                          DEFAULT_CHUNK_SIZE );
                    node_info[x+1][y].ids[2] = *n_points-1;
                }
                node_info[x][y].ids[0] = node_info[x+1][y].ids[2];
            }
            else if( this_value * neigh_value < 0.0 )
            {
                fraction = this_value / (this_value - neigh_value);
                get_point( volume, axis_index, slice_pos,
                           (VIO_Real) x + fraction, (VIO_Real) y, &point );
                ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                      DEFAULT_CHUNK_SIZE );
                node_info[x][y].ids[0] = *n_points-1;
            }

            this_value = values[x][y];
            neigh_value = values[x][y+1];

            if( this_value == 0.0 && neigh_value > 0.0 )
            {
                if( node_info[x][y].ids[2] < 0 )
                {
                    get_point( volume, axis_index, slice_pos,
                               (VIO_Real) x, (VIO_Real) y, &point );
                    ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                          DEFAULT_CHUNK_SIZE );
                    node_info[x][y].ids[2] = *n_points-1;
                }
                node_info[x][y].ids[0] = node_info[x][y].ids[2];
            }
            else if( this_value > 0.0 && neigh_value == 0.0 )
            {
                if( node_info[x][y+1].ids[2] < 0 )
                {
                    get_point( volume, axis_index, slice_pos,
                               (VIO_Real) x, (VIO_Real) y+1.0, &point );
                    ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                          DEFAULT_CHUNK_SIZE );
                    node_info[x][y+1].ids[2] = *n_points-1;
                }
                node_info[x][y].ids[0] = node_info[x][y+1].ids[2];
            }
            else if( this_value * neigh_value < 0.0 )
            {
                fraction = this_value / (this_value - neigh_value);
                get_point( volume, axis_index, slice_pos,
                           (VIO_Real) x, (VIO_Real) y + fraction, &point );
                ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                      DEFAULT_CHUNK_SIZE );
                node_info[x][y].ids[1] = *n_points-1;
            }


            neigh_value = (values[x][y] + values[x+1][y] + values[x][y+1] +
                           values[x+1][y+1]) / 4.0;

            node_info[x][y].lower_left_fence = (VIO_SCHAR)
                                       (this_value * neigh_value <= 0.0);
        }
    }
}

static  void  contour_rectangle(
    int           x,
    int           y,
    node_struct   **node_info,
    lines_struct  *lines )
{
    int   i, j, n_nodes, nodes[4], edges[4], n_indices, p1, p2, p3, p4;

    edges[0] = node_info[x][y].ids[0];
    edges[1] = node_info[x+1][y].ids[1];
    edges[2] = node_info[x][y+1].ids[0];
    edges[3] = node_info[x][y].ids[1];

    for_less( i, 0, 3 )
    {
        if( edges[i] < 0 )
            continue;

        for_less( j, i+1, 4 )
        {
            if( edges[i] == edges[j] )
            {
                edges[i] = -1;
                edges[j] = -1;
                break;
            }
        }
    }

    n_nodes = 0;

    for_less( i, 0, 4 )
    {
        if( edges[i] >= 0 )
        {
            nodes[n_nodes] = edges[i];
            ++n_nodes;
        }
    }


    if( n_nodes == 2 )
    {
        n_indices = NUMBER_INDICES( *lines );

        ADD_ELEMENT_TO_ARRAY( lines->end_indices, lines->n_items,
                              n_indices + 2, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( lines->indices, n_indices,
                              nodes[0], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( lines->indices, n_indices,
                              nodes[1], DEFAULT_CHUNK_SIZE );
    }
    else  if( n_nodes == 4 )
    {
        if( node_info[x][y].lower_left_fence )
        {
            p1 = edges[0];
            p2 = edges[3];
            p3 = edges[1];
            p4 = edges[2];
        }
        else
        {
            p1 = edges[0];
            p2 = edges[1];
            p3 = edges[2];
            p4 = edges[3];
        }

        n_indices = NUMBER_INDICES( *lines );

        ADD_ELEMENT_TO_ARRAY( lines->end_indices, lines->n_items,
                              n_indices + 2, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( lines->indices, n_indices,
                              p1, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( lines->indices, n_indices,
                              p2, DEFAULT_CHUNK_SIZE );

        ADD_ELEMENT_TO_ARRAY( lines->end_indices, lines->n_items,
                              n_indices + 2, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( lines->indices, n_indices,
                              p3, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( lines->indices, n_indices,
                              p4, DEFAULT_CHUNK_SIZE );
    }
}

static  void  contour_volume_slice(
    VIO_Volume        volume,
    int           axis_index,
    VIO_Real          slice_pos,
    VIO_BOOL       world_flag,
    VIO_Real          contour_value,
    lines_struct  *lines )
{
    int           x, y, a1, a2, sizes[VIO_N_DIMENSIONS];
    node_struct   **node_info;
    VIO_Real          **values;
    VIO_Real          world[VIO_N_DIMENSIONS], voxel[VIO_N_DIMENSIONS];

    a1 = (axis_index + 1) % VIO_N_DIMENSIONS;
    a2 = (axis_index + 2) % VIO_N_DIMENSIONS;

    if( world_flag )
    {
        world[VIO_X] = 0.0;
        world[VIO_Y] = 0.0;
        world[VIO_Z] = 0.0;
        world[axis_index] = slice_pos;
        convert_world_to_voxel( volume, world[VIO_X], world[VIO_Y], world[VIO_Z],
                                voxel );
        slice_pos = voxel[axis_index];
    }

    get_volume_sizes( volume, sizes );

    VIO_ALLOC2D( values, sizes[a1], sizes[a2] );

    record_values( volume, a1, a2, axis_index, slice_pos,
                   sizes[a1], sizes[a2], contour_value, values );

    VIO_ALLOC2D( node_info, sizes[a1], sizes[a2] );

    create_node_points( volume, axis_index, slice_pos,
                        sizes[a1], sizes[a2], values, node_info,
                        &lines->n_points, &lines->points );

    VIO_FREE2D( values );

    for_less( x, 0, sizes[a1] - 1 )
    {
        for_less( y, 0, sizes[a2] - 1 )
        {
            contour_rectangle( x, y, node_info, lines );
        }
    }

    print( "Contour: %g   N points: %d    N lines: %d\n",
           contour_value, lines->n_points, lines->n_items );

    VIO_FREE2D( node_info );
}
