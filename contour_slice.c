#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  contour_volume_slice(
    Volume        volume,
    int           axis_index,
    int           slice_index,
    Real          contour_value,
    lines_struct  *lines );

private  int  get_axis_index(
    STRING   name,
    int      *axis_index )
{
    *axis_index = -1;

    switch( name[0] )
    {
    case 'x':
    case 'X': *axis_index = X; break;
    case 'y':
    case 'Y': *axis_index = Y; break;
    case 'z':
    case 'Z': *axis_index = Z; break;
    }

    return( *axis_index >= 0 );
}

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s  volume_file  output_file  x|y|z slice_pos\n\
       [contour_value1] [contour_value2] ... \n\
\n\
     Creates contour lines for the given slice at the given contour values.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_volume_filename, output_filename;
    STRING               axis_name;
    Volume               volume;
    int                  n_objects, slice_index, axis_index;
    object_struct        **objects, *object;
    Real                 contour_value;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_string_argument( "", &axis_name ) ||
        !get_axis_index( axis_name, &axis_index ) ||
        !get_int_argument( 0, &slice_index ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    n_objects = 0;

    while( get_real_argument( 0.0, &contour_value ) )
    {
        object = create_object( LINES );

        initialize_lines( get_lines_ptr(object), WHITE );

        contour_volume_slice( volume, axis_index, slice_index, contour_value,
                              get_lines_ptr(object) );

        add_object_to_list( &n_objects, &objects, object );
    }

    if( output_graphics_file( output_filename, ASCII_FORMAT, n_objects,
                              objects ) != OK )
        return( 1 );

    delete_object_list( n_objects, objects );
    delete_volume( volume );

    return( 0 );
}

typedef  struct
{
    int            ids[3];
    Smallest_int   lower_left_fence;
} node_struct;

private  void  record_values(
    Volume        volume,
    int           a1,
    int           a2,
    int           axis_index,
    int           slice_index,
    int           x_size,
    int           y_size,
    Real          contour_value,
    Real          **values )
{
    int  x, y, voxel[N_DIMENSIONS];

    voxel[axis_index] = slice_index;
    for_less( x, 0, x_size )
    {
        voxel[a1] = x;
        for_less( y, 0, y_size )
        {
            voxel[a2] = y;
            values[x][y] = get_volume_real_value( volume,
                                 voxel[X], voxel[Y], voxel[Z], 0, 0 )
                           - contour_value;
        }
    }
}

private  void  get_point(
    Volume   volume,
    int      axis_index,
    int      slice_index,
    Real     x,
    Real     y,
    Point    *point )
{
    Real   voxel[N_DIMENSIONS], xw, yw, zw;

    voxel[axis_index] = (Real) slice_index;
    voxel[(axis_index+1) % N_DIMENSIONS] = x;
    voxel[(axis_index+2) % N_DIMENSIONS] = y;

    convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );

    fill_Point( *point, xw, yw, zw );
}

private  void  create_node_points(
    Volume       volume,
    int          axis_index,
    int          slice_index,
    int          x_size,
    int          y_size,
    Real         **values,
    node_struct  **node_info,
    int          *n_points,
    Point        *points[] )
{
    int    x, y;
    Real   this_value, neigh_value, fraction;
    Point  point;

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
                    get_point( volume, axis_index, slice_index,
                               (Real) x, (Real) y, &point );
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
                    get_point( volume, axis_index, slice_index,
                               (Real) x+1.0, (Real) y, &point );
                    ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                          DEFAULT_CHUNK_SIZE );
                    node_info[x+1][y].ids[2] = *n_points-1;
                }
                node_info[x][y].ids[0] = node_info[x+1][y].ids[2];
            }
            else if( this_value * neigh_value < 0.0 )
            {
                fraction = this_value / (this_value - neigh_value);
                get_point( volume, axis_index, slice_index,
                           (Real) x + fraction, (Real) y, &point );
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
                    get_point( volume, axis_index, slice_index,
                               (Real) x, (Real) y, &point );
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
                    get_point( volume, axis_index, slice_index,
                               (Real) x, (Real) y+1.0, &point );
                    ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                          DEFAULT_CHUNK_SIZE );
                    node_info[x][y+1].ids[2] = *n_points-1;
                }
                node_info[x][y].ids[0] = node_info[x][y+1].ids[2];
            }
            else if( this_value * neigh_value < 0.0 )
            {
                fraction = this_value / (this_value - neigh_value);
                get_point( volume, axis_index, slice_index,
                           (Real) x, (Real) y + fraction, &point );
                ADD_ELEMENT_TO_ARRAY( *points, *n_points, point,
                                      DEFAULT_CHUNK_SIZE );
                node_info[x][y].ids[1] = *n_points-1;
            }


            neigh_value = (values[x][y] + values[x+1][y] + values[x][y+1] +
                           values[x+1][y+1]) / 4.0;

            node_info[x][y].lower_left_fence = (Smallest_int)
                                       (this_value * neigh_value <= 0.0);
        }
    }
}

private  void  contour_rectangle(
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

private  void  contour_volume_slice(
    Volume        volume,
    int           axis_index,
    int           slice_index,
    Real          contour_value,
    lines_struct  *lines )
{
    int           x, y, a1, a2, sizes[N_DIMENSIONS];
    node_struct   **node_info;
    Real          **values;

    a1 = (axis_index + 1) % N_DIMENSIONS;
    a2 = (axis_index + 2) % N_DIMENSIONS;

    get_volume_sizes( volume, sizes );

    ALLOC2D( values, sizes[a1], sizes[a2] );

    record_values( volume, a1, a2, axis_index, slice_index,
                   sizes[a1], sizes[a2], contour_value, values );

    ALLOC2D( node_info, sizes[a1], sizes[a2] );

    create_node_points( volume, axis_index, slice_index,
                        sizes[a1], sizes[a2], values, node_info,
                        &lines->n_points, &lines->points );

    FREE2D( values );

    for_less( x, 0, sizes[a1] - 1 )
    {
        for_less( y, 0, sizes[a2] - 1 )
        {
            contour_rectangle( x, y, node_info, lines );
        }
    }

    print( "Contour: %g   N points: %d    N lines: %d\n",
           contour_value, lines->n_points, lines->n_items );

    FREE2D( node_info );
}
