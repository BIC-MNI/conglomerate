#include  <module.h>

#define  DEFAULT_N_INTERVALS  255

#define  WINDOW_WIDTH 0.05

#define  MAX_POINTS   10

private  BOOLEAN  is_maximum(
    int     counts[],
    int     i,
    int     n,
    int     width );
private  BOOLEAN  is_minimum(
    int     counts[],
    int     i,
    int     n,
    int     width );

int  main(
    int   argc,
    char  *argv[] )
{
    int                  x, y, z, sizes[N_DIMENSIONS], x_size, y_size;
    int                  start[N_DIMENSIONS], end[N_DIMENSIONS];
    int                  n, i, *counts, window_width, slice, axis;
    Real                 value, width_ratio, min_voxel, max_voxel, voxel;
    Real                 min_value, max_value;
    char                 *input_volume_filename, *output_filename;
    char                 *axis_name;
    lines_struct         *lines;
    object_struct        *object;
    histogram_struct     histogram;
    int                  mins[MAX_POINTS], maxs[MAX_POINTS];
    int                  n_mins, n_maxs, end_gray_index, start_gray_index;
    Volume               volume;

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

    (void) get_int_argument( DEFAULT_N_INTERVALS, &x_size );
    (void) get_int_argument( x_size, &y_size );
    (void) get_real_argument( WINDOW_WIDTH, &width_ratio );

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

    n = (int) max_voxel - (int) min_voxel + 1;
    ALLOC( counts, n );
    for_less( i, 0, n )
        counts[i] = 0;

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
                GET_VALUE_3D( value, volume, x, y, z );
                add_to_histogram( &histogram, value );

                GET_VOXEL_3D( voxel, volume, x, y, z );
                ++counts[(int)voxel - (int)min_voxel];
            }
        }
    }

    object = create_object( LINES );

    lines = get_lines_ptr( object );

    create_histogram_line( &histogram, x_size, y_size, lines );

    (void) output_graphics_file( output_filename, ASCII_FORMAT, 1, &object );

    window_width = ROUND( n * width_ratio );
    if( window_width < 1 )
        window_width = 1;

    n_mins = 0;
    n_maxs = 0;
    for_less( i, 0, n )
    {
        if( is_minimum( counts, i, n, window_width ) )
        {
            value = CONVERT_VOXEL_TO_VALUE( volume, i + (int) min_voxel );
            print( "Minimum at %g\n", value );
            if( n_mins < MAX_POINTS )
                mins[n_mins] = i;
            ++n_mins;
        }
        else if( is_maximum( counts, i, n, window_width ) )
        {
            value = CONVERT_VOXEL_TO_VALUE( volume, i + (int) min_voxel );
            print( "     Maximum at %g\n", value );
            if( n_maxs < MAX_POINTS )
                maxs[n_maxs] = i;
            ++n_maxs;
        }
    }

    start_gray_index = end_gray_index - 2;

    while( start_gray_index > 0 &&
           counts[start_gray_index] >= counts[end_gray_index] )
        --start_gray_index;

    print( "Gray matter: %g %g\n",
            CONVERT_VOXEL_TO_VALUE(volume,
                                   (mins[1] + maxs[1]) / 2.0 + (int) min_voxel),
            CONVERT_VOXEL_TO_VALUE(volume, mins[2] + (int) min_voxel));

    return( 0 );
}

private  BOOLEAN  is_minimum(
    int     counts[],
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
        if( counts[d] <= counts[i] )
            return( FALSE );

    return( TRUE );
}

private  BOOLEAN  is_maximum(
    int     counts[],
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
        if( counts[d] >= counts[i] )
            return( FALSE );

    return( TRUE );
}
