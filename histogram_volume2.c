#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  FILTER_WIDTH 0.02

#define  DEFAULT_Y_SCALE  10.0

#define  MAX_BOXES        40000

#define  DEFAULT_N_BOXES  -1

int  main(
    int   argc,
    char  *argv[] )
{
    int                  x, y, z, sizes[N_DIMENSIONS];
    int                  start[N_DIMENSIONS], end[N_DIMENSIONS];
    int                  slice, axis, xyz_axis;
    Real                 value, min_voxel, max_voxel, filter_width;
    Real                 min_value, max_value, filter_ratio;
    Real                 min_nonzero, max_nonzero, y_scale;
    Real                 min_range, max_range;
    STRING               input_volume_filename, output_filename;
    STRING               axis_name;
    lines_struct         *lines;
    histogram_struct     histogram;
    Real                 xyz[N_DIMENSIONS], voxel[N_DIMENSIONS];
    Real                 *counts, delta;
    Real                 scale, trans, interval_width;
    Real                 x1, y1, x2, y2;
    int                  n, i;
    int                  n_objects, n_voxels;
    int                  n_boxes, deriv;
    object_struct        **objects;
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) )
    {
        print_error("Usage: %s volume_file output_filename|none [x|y|z|none]\n",
                     argv[0] );
        print_error( "   [slice_pos]  [filter_ratio] [y_scale] [n_boxes]\n" );
        return( 1 );
    }

    (void) get_string_argument( "none", &output_filename );
    (void) get_string_argument( "none", &axis_name );
    (void) get_int_argument( 0, &slice );
    (void) get_real_argument( FILTER_WIDTH, &filter_ratio );
    (void) get_real_argument( DEFAULT_Y_SCALE, &y_scale );
    (void) get_int_argument( DEFAULT_N_BOXES, &n_boxes );
    (void) get_real_argument( 0.0, &min_range );
    (void) get_real_argument( min_range-1.0, &max_range );
    (void) get_int_argument( 0, &deriv );

    if( axis_name[0] == 'x' || axis_name[0] == 'X' )
        axis = X;
    else if( axis_name[0] == 'y' || axis_name[0] == 'Y' )
        axis = Y;
    else if( axis_name[0] == 'z' || axis_name[0] == 'Z' )
        axis = Z;
    else
        axis = -1;

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

    if( min_range < max_range )
    {
        min_value = min_range;
        max_value = max_range;
    }

    if( n_boxes >= 1 )
        scale = (max_value - min_value) / (Real) (n_boxes-1);
    else
        scale = (max_value - min_value) / (max_voxel - min_voxel);

    delta = scale;
    if( n_boxes < 1 && (max_value - min_value) / delta > (Real) MAX_BOXES )
        delta = (max_value - min_value) / (Real) MAX_BOXES;

    initialize_histogram( &histogram, delta, min_value - scale/2.0 );

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

    n_voxels = 0;
    for_less( x, start[X], end[X] )
    {
        for_less( y, start[Y], end[Y] )
        {
            for_less( z, start[Z], end[Z] )
            {
                value = get_volume_real_value( volume, x, y, z, 0, 0 );
                if( min_value <= value && value <= max_value )
                {
                    add_to_histogram( &histogram, value );
                    ++n_voxels;
                }
            }
        }
    }

    /*--- find mins and maxes */

    filter_width = 0.0;

    n = get_histogram_counts( &histogram, &counts, filter_width,
                              &scale, &trans );

    min_nonzero = scale * (-0.5) + trans;
    max_nonzero = scale * ((Real) n + 0.5) + trans;

    FREE( counts );

    filter_width = filter_ratio * (max_nonzero - min_nonzero);

    n = get_histogram_counts( &histogram, &counts, filter_width,
                              &scale, &trans );

    interval_width = 1.0 / (Real) n;

    n_objects = 1;
    ALLOC( objects, n_objects );
    objects[0] = create_object( LINES );
    lines = get_lines_ptr( objects[0] );

    initialize_lines( lines, WHITE );

    lines->n_points = n;
    ALLOC( lines->points, n );

    for_less( i, 0, n )
    {
        fill_Point( lines->points[i], (Real) i * scale + trans,
                    (Real) counts[i] / interval_width /
                    (Real) n_voxels, 0.0 );
    }

    if( deriv == 1 )
    {
        for_less( i, 0, n-1 )
        {
            x1 = (Real) Point_x(lines->points[i]);
            y1 = (Real) Point_y(lines->points[i]);
            x2 = (Real) Point_x(lines->points[i+1]);
            y2 = (Real) Point_y(lines->points[i+1]);

            print( "%g %g\n", x1, y1 );

            Point_y(lines->points[i]) = (Point_coord_type)
                                         (y_scale * (y2 - y1) / (x2-x1));
        }

        Point_y(lines->points[n-1]) = Point_y(lines->points[n-2]);
    }
    else
    {
        for_less( i, 0, n )
            Point_y(lines->points[i]) *= (Point_coord_type) y_scale;
    }

    lines->n_items = 1;
    ALLOC( lines->end_indices, 1 );
    lines->end_indices[0] = n;

    ALLOC( lines->indices, n );
    for_less( i, 0, n )
        lines->indices[i] = i;

    (void) output_graphics_file( output_filename, ASCII_FORMAT,
                                 n_objects, objects );

    return( 0 );
}
