#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_N_INTERVALS  255

#define  FILTER_WIDTH 0.0
#define  WINDOW_WIDTH 0.05

#define  MAX_POINTS   10

#ifdef OLD
private  BOOLEAN  is_maximum(
    Real    counts[],
    int     i,
    int     n,
    int     width );
private  BOOLEAN  is_minimum(
    Real    counts[],
    int     i,
    int     n,
    int     width );
#endif

int  main(
    int   argc,
    char  *argv[] )
{
    int                  x, y, z, sizes[N_DIMENSIONS], x_size, y_size;
    int                  start[N_DIMENSIONS], end[N_DIMENSIONS];
    int                  slice, axis;
    Real                 value, min_voxel, max_voxel, window_width;
    Real                 min_value, max_value, filter_ratio;
    STRING               input_volume_filename, output_filename;
    STRING               axis_name;
    lines_struct         *lines;
    histogram_struct     histogram;
#ifdef OLD
    Real                 scale, trans;
    int                  gray_min;
    Real                 *counts, width_ratio;
    int                  n, i, mins[MAX_POINTS], maxs[MAX_POINTS];
    int                  n_mins, n_maxs, end_gray_index, start_gray_index;
    int                  int_width;
#endif
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

#ifdef OLD
    (void) get_real_argument( WINDOW_WIDTH, &width_ratio );
#endif

    (void) get_int_argument( DEFAULT_N_INTERVALS, &x_size );
    (void) get_int_argument( x_size, &y_size );
    put_x_pos = get_real_argument( 0.0, &x_pos );

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
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

    if( axis >= 0 && slice >= 0 && slice < sizes[axis] )
    {
        start[axis] = slice;
        end[axis] = slice+1;
        print( "Slice %d in %c\n", slice, "XYZ"[axis] );
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

    n_objects = 1;
    ALLOC( objects, 2 );

    objects[0] = create_object( LINES );
    lines = get_lines_ptr( objects[0] );
    create_histogram_line( &histogram, x_size, y_size, window_width, lines );

    if( put_x_pos )
    {
        min_pos = (Real) Point_x(lines->points[0]);
        max_pos = (Real) Point_x(lines->points[lines->n_points-1]);

        ++n_objects;
        objects[1] = create_object( LINES );
        lines = get_lines_ptr( objects[1] );
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
    }

    (void) output_graphics_file( output_filename, ASCII_FORMAT,
                                 n_objects, objects );

#ifdef OLD
    n = get_histogram_counts( &histogram, &counts, window_width,
                              &scale, &trans );

    int_width = ROUND( (Real) x_size * width_ratio / 2.0 );
    if( int_width < 1 )
        int_width = 1;

    n_mins = 0;
    n_maxs = 0;
    for_less( i, 0, n )
    {
        if( is_minimum( counts, i, n, int_width ) )
        {
            value = scale * (Real) i + trans;
            print( "Minimum at %g\n", value );
            if( n_mins < MAX_POINTS )
                mins[n_mins] = i;
            ++n_mins;
        }
        else if( is_maximum( counts, i, n, int_width ) )
        {
            value = scale * (Real) i + trans;
            print( "     Maximum at %g\n", value );
            if( n_maxs < MAX_POINTS )
                maxs[n_maxs] = i;
            ++n_maxs;
        }
    }

    gray_min = 0;
    while( mins[gray_min] < maxs[0] )
        ++gray_min;

    end_gray_index = mins[gray_min+1];
    start_gray_index = end_gray_index - 1;
    while( start_gray_index >= mins[gray_min] &&
           counts[start_gray_index] >= counts[end_gray_index] )
    {
         --start_gray_index;
    }

    print( "Gray matter: %g %g\n",
            scale * (Real) start_gray_index + trans,
            scale * (Real) end_gray_index + trans );

    FREE( counts );
#endif

    return( 0 );
}

#ifdef OLD
private  BOOLEAN  is_minimum(
    Real    counts[],
    int     i,
    int     n,
    int     width )
{
    int   d, m1, m2;

    if( i != 0 && counts[i-1] <= counts[i] ||
        i < n-1 && counts[i+1] <= counts[i] )
        return( FALSE );

    m1 = MAX( 0, i - width );
    m2 = MIN( n-1, i + width );

    for_inclusive( d, m1, m2 )
        if( d != i && counts[d] <= counts[i] )
            return( FALSE );

    return( TRUE );
}

private  BOOLEAN  is_maximum(
    Real    counts[],
    int     i,
    int     n,
    int     width )
{
    int   d, m1, m2;

    if( i != 0 && counts[i-1] >= counts[i] ||
        i < n-1 && counts[i+1] >= counts[i] )
        return( FALSE );

    m1 = MAX( 0, i - width );
    m2 = MIN( n-1, i + width );

    for_inclusive( d, m1, m2 )
        if( d != i && counts[d] >= counts[i] )
            return( FALSE );

    return( TRUE );
}
#endif
