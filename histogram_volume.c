#include  <module.h>

#define  DEFAULT_N_INTERVALS  255

#define  WINDOW_WIDTH 0.05

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
    int                  n, i, *counts, window_width;
    Real                 value, width_ratio;
    Real                 min_value, max_value, scale, trans;
    char                 *input_volume_filename, *output_filename;
    lines_struct         *lines;
    object_struct        *object;
    histogram_struct     histogram;
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  volume_file output_file\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( DEFAULT_N_INTERVALS, &x_size );
    (void) get_int_argument( x_size, &y_size );
    (void) get_real_argument( WINDOW_WIDTH, &width_ratio );

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );
    get_volume_real_range( volume, &min_value, &max_value );

    initialize_histogram( &histogram,
                          (max_value - min_value) / (Real) (x_size-1),
                          min_value );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VALUE_3D( value, volume, x, y, z );
                add_to_histogram( &histogram, value );
            }
        }
    }

    object = create_object( LINES );

    lines = get_lines_ptr( object );

    create_histogram_line( &histogram, x_size, y_size, lines );

    (void) output_graphics_file( output_filename, ASCII_FORMAT, 1, &object );

    window_width = ROUND( x_size * width_ratio );
    if( window_width < 1 )
        window_width = 1;

    n = get_histogram_counts( &histogram, &counts, &scale, &trans );

    for_less( i, 0, n )
    {
        if( is_minimum( counts, i, n, window_width ) )
            print( "Minimum at %g\n", scale * (Real) i + trans );
        else if( is_maximum( counts, i, n, window_width ) )
            print( "   Maximum at %g\n", scale * (Real) i + trans );
    }

    return( 0 );
}

private  BOOLEAN  is_minimum(
    int     counts[],
    int     i,
    int     n,
    int     width )
{
    int   d, m1, m2;

    if( i != 0 && counts[i-1] < counts[i] ||
        i < n-1 && counts[i+1] < counts[i] )
        return( FALSE );

    m1 = MAX( 0, i - width );
    m2 = MIN( n-1, i + width );

    for_inclusive( d, m1, m2 )
        if( counts[d] < counts[i] )
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

    if( i != 0 && counts[i-1] > counts[i] ||
        i < n-1 && counts[i+1] > counts[i] )
        return( FALSE );

    m1 = MAX( 0, i - width );
    m2 = MIN( n-1, i + width );

    for_inclusive( d, m1, m2 )
        if( counts[d] > counts[i] )
            return( FALSE );

    return( TRUE );
}
