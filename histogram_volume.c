#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_N_INTERVALS  100

#define  FILTER_WIDTH 0.0
#define  WINDOW_WIDTH 0.05

#define  MAX_POINTS   10

int  main(
    int   argc,
    char  *argv[] )
{
    int                  x, y, z, sizes[N_DIMENSIONS], x_size, y_size;
    int                  start[N_DIMENSIONS], end[N_DIMENSIONS];
    int                  slice, axis, xyz_axis;
    Real                 value, min_voxel, max_voxel, window_width;
    Real                 min_value, max_value, filter_ratio;
    STRING               input_volume_filename, output_filename;
    STRING               axis_name;
    lines_struct         *lines;
    histogram_struct     histogram;
    Real                 grad, max_grad;
    Real                 xyz[N_DIMENSIONS], voxel[N_DIMENSIONS];
    Real                 *counts, pos_low, pos_max_grad, pos_high;
    Real                 scale, trans;
    int                  n, i, min_index, max_index, max_index2;
    int                  n_objects;
    object_struct        **objects;
    Volume               volume;
    Real                 x_pos, y_height, pos, min_pos, max_pos;
    BOOLEAN              put_x_pos;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_string_argument( "", &axis_name ) ||
        !get_int_argument( 0, &slice ) )
    {
        print( "Usage: %s  volume_file output_file\n", argv[0] );
        return( 1 );
    }

    if( axis_name[0] == 'x' || axis_name[0] == 'X' )
        axis = X;
    else if( axis_name[0] == 'y' || axis_name[0] == 'Y' )
        axis = Y;
    else if( axis_name[0] == 'z' || axis_name[0] == 'Z' )
        axis = Z;
    else
        axis = -1;

    (void) get_real_argument( FILTER_WIDTH, &filter_ratio );
    (void) get_int_argument( DEFAULT_N_INTERVALS, &x_size );
    (void) get_int_argument( x_size, &y_size );
    put_x_pos = get_real_argument( 0.0, &x_pos );

    set_n_bytes_cache_threshold( 1 );
    set_default_max_bytes_in_cache( 1 );
    set_cache_block_sizes_hint( SLICE_ACCESS );

    if( input_volume( input_volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );
    get_volume_voxel_range( volume, &min_voxel, &max_voxel );
    get_volume_real_range( volume, &min_value, &max_value );

    initialize_histogram( &histogram,
                          (max_value - min_value) / (Real) (x_size-1),
                          min_value );

    start[X] = 0;
    end[X] = sizes[X];
    start[Y] = 0;
    end[Y] = sizes[Y];
    start[Z] = 0;
    end[Z] = sizes[Z];

    if( axis >= 0 && axis < N_DIMENSIONS )
    {
        xyz_axis = axis;
        xyz[0] = 0.0;
        xyz[1] = 0.0;
        xyz[2] = 0.0;
        xyz[axis] = 1.0;
        reorder_xyz_to_voxel( volume, xyz, voxel );
        for_less( axis, 0, N_DIMENSIONS )
        {
            if( voxel[axis] == 1.0 )
                break;
        }

        if( slice >= 0 && slice < sizes[axis] )
        {
            start[axis] = slice;
            end[axis] = slice+1;
            print( "Slice %d in %c\n", slice, "XYZ"[xyz_axis] );
        }
    }

    for_less( x, start[X], end[X] )
    {
        for_less( y, start[Y], end[Y] )
        {
            for_less( z, start[Z], end[Z] )
            {
                value = get_volume_real_value( volume, x, y, z, 0, 0 );
                add_to_histogram( &histogram, value );
            }
        }
    }

    window_width = filter_ratio * (max_value - min_value);

    /*--- find mins and maxes */

    n = get_histogram_counts( &histogram, &counts, window_width,
                              &scale, &trans );

    max_index = 0;
    for_less( i, 0, n )
    {
        if( i == 0 || counts[i] > counts[max_index] )
            max_index = i;
    }

    i = max_index;
    while( i < n && counts[i] > counts[max_index] / 2.0 )
        ++i;

    while( i < n && counts[i] > counts[i+1] )
        ++i;

    max_index2 = i;

    for( ; i < n;  ++i )
    {
        if( counts[i] > counts[max_index2] )
            max_index2 = i;
    }

    min_index = 0;

    for_less( i, max_index, max_index2 )
    {
        if( i == max_index || counts[i] < counts[min_index] )
            min_index = i;
    }

    pos_max_grad = 0.0;
    max_grad = 0.0;

    for_less( i, min_index, max_index2 )
    {
        grad = counts[i+1] - counts[i];
        if( i == min_index || grad > max_grad )
        {
            pos_max_grad = scale * ((Real) i + 0.5) + trans;
            max_grad = grad;
        }
    }

    pos_low = scale * (Real) min_index + trans;
    pos_high = scale * (Real) max_index2 + trans;

    n_objects = 1;
    ALLOC( objects, 4 );

    objects[0] = create_object( LINES );
    lines = get_lines_ptr( objects[0] );
    create_histogram_line( &histogram, x_size, y_size, window_width, lines );

    print( "Positions %g %g %g\n", pos_low, pos_max_grad, pos_high );

    if( put_x_pos )
    {
        min_pos = (Real) Point_x(lines->points[0]);
        max_pos = (Real) Point_x(lines->points[lines->n_points-1]);

        objects[n_objects] = create_object( LINES );
        lines = get_lines_ptr( objects[n_objects] );
        initialize_lines( lines, RED );
        lines->n_points = 2;
        ALLOC( lines->points, 2 );

        pos = min_pos + (max_pos - min_pos) *
                        (x_pos - min_value) / (max_value - min_value);

        y_height = 0.05 * (max_pos - min_pos);
        fill_Point( lines->points[0], pos, -y_height, 0.0 );
        fill_Point( lines->points[1], pos, y_height, 0.0 );
        lines->n_items = 1;
        ALLOC( lines->end_indices, 1 );
        lines->end_indices[0] = 2;
        ALLOC( lines->indices, 2 );
        lines->indices[0] = 0;
        lines->indices[1] = 1;
        ++n_objects;
    }

    objects[n_objects] = create_object( LINES );
    lines = get_lines_ptr( objects[n_objects] );
    initialize_lines( lines, BLUE );
    lines->n_points = 2;
    ALLOC( lines->points, 2 );

    pos = min_pos + (max_pos - min_pos) *
                    (pos_low - min_value) / (max_value - min_value);

    y_height = 0.05 * (max_pos - min_pos);
    fill_Point( lines->points[0], pos, -y_height, 0.0 );
    fill_Point( lines->points[1], pos, y_height, 0.0 );
    lines->n_items = 1;
    ALLOC( lines->end_indices, 1 );
    lines->end_indices[0] = 2;
    ALLOC( lines->indices, 2 );
    lines->indices[0] = 0;
    lines->indices[1] = 1;
    ++n_objects;

    objects[n_objects] = create_object( LINES );
    lines = get_lines_ptr( objects[n_objects] );
    initialize_lines( lines, GREEN );
    lines->n_points = 2;
    ALLOC( lines->points, 2 );

    pos = min_pos + (max_pos - min_pos) *
                    (pos_high - min_value) / (max_value - min_value);

    y_height = 0.05 * (max_pos - min_pos);
    fill_Point( lines->points[0], pos, -y_height, 0.0 );
    fill_Point( lines->points[1], pos, y_height, 0.0 );
    lines->n_items = 1;
    ALLOC( lines->end_indices, 1 );
    lines->end_indices[0] = 2;
    ALLOC( lines->indices, 2 );
    lines->indices[0] = 0;
    lines->indices[1] = 1;
    ++n_objects;

    if( string_length( output_filename ) > 0 &&
        !equal_strings( output_filename, "none" ) )
    {
        (void) output_graphics_file( output_filename, ASCII_FORMAT,
                                     n_objects, objects );
    }

    return( 0 );
}
