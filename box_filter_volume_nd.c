#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  box_filter_volume(
    Volume   volume,
    Real     filter_widths[] );

private  void  usage(
    STRING   executable )
{
    STRING   usage_str = "\n\
Usage: %s input.mnc output.mnc x_width y_width z_width [time_width]\n\
\n\
     Box filters a volume with the given WORLD widths,\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    int        dim, n_dims;
    Volume     volume;
    Real       file_order_filter_widths[MAX_DIMENSIONS];
    Real       filter_widths[MAX_DIMENSIONS];
    Real       separations[MAX_DIMENSIONS];
    STRING     input_filename, output_filename, history;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }


    if( input_volume( input_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    n_dims = get_volume_n_dimensions( volume );

    for_less( dim, 0, n_dims )
        (void) get_real_argument( 0.0, &filter_widths[dim] );

    reorder_xyz_to_voxel( volume, filter_widths, file_order_filter_widths );

    get_volume_separations( volume, separations );
    for_less( dim, 0, n_dims )
        file_order_filter_widths[dim] /= FABS( separations[dim] );

    box_filter_volume( volume, file_order_filter_widths );

    history = "box_filter_volume_nd ...\n";

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, volume, history, NULL );


    return( 0 );
}

private  void  box_filter_1d(
    int   size,
    Real  values[],
    Real  output[],
    Real  width )
{
    int   start, end, v;
    Real  left_weight, right_weight, half_width, current;

    half_width = width / 2.0;

    left_weight = FRACTION( half_width + 0.5 );
    right_weight = 1.0 - left_weight;

    current = 0.0;

    start = FLOOR( -half_width + 0.5 );
    end = FLOOR( half_width + 0.5 );

    for_less( v, 0, MIN(end,size) )
        current += values[v];
    if( end <= size-1 )
        current += left_weight * values[end];

    for_less( v, 0, size )
    {
        output[v] = current / width;
        if( end < size )
            current += right_weight * values[end];
        ++end;
        if( end < size )
            current += left_weight * values[end];

        if( start >= 0 )
            current -= left_weight * values[start];
        ++start;
        if( start >= 0 )
            current -= right_weight * values[start];
    }
}

private  void  box_filter_volume(
    Volume   volume,
    Real     filter_widths[] )
{
    int   dim, n_dims, voxel[MAX_DIMENSIONS], blurring_dim;
    int   sizes[MAX_DIMENSIONS], max_size;
    int   start[MAX_DIMENSIONS];
    int   end[MAX_DIMENSIONS];
    int   count[MAX_DIMENSIONS];
    Real  *values, *output, volume_min, volume_max, value;

    n_dims = get_volume_n_dimensions( volume );
    get_volume_sizes( volume, sizes );

    get_volume_real_range( volume, &volume_min, &volume_max );

    max_size = 0;
    for_less( dim, 0, n_dims )
        max_size = MAX( max_size, sizes[dim] );

    ALLOC( values, max_size );
    ALLOC( output, max_size );

    for_less( dim, n_dims, MAX_DIMENSIONS )
    {
        start[dim] = 0;
        end[dim] = 0;
        count[dim] = 1;
    }

    for_less( blurring_dim, 0, n_dims )
    {
        if( filter_widths[blurring_dim] <= 1.0 )
            continue;

        for_less( dim, 0, n_dims )
        {
            start[dim] = 0;
            end[dim] = sizes[dim]-1;
            count[dim] = 1;
        }

        end[blurring_dim] = 0;
        count[blurring_dim] = sizes[blurring_dim];

        for_inclusive( voxel[0], start[0], end[0] )
        for_inclusive( voxel[1], start[1], end[1] )
        for_inclusive( voxel[2], start[2], end[2] )
        for_inclusive( voxel[3], start[3], end[3] )
        for_inclusive( voxel[4], start[4], end[4] )
        {
            get_volume_value_hyperslab( volume, voxel[0], voxel[1], voxel[2],
                                        voxel[3], voxel[4],
                                        count[0], count[1], count[2],
                                        count[3], count[4], values );

            box_filter_1d( sizes[blurring_dim], values, output,
                           filter_widths[blurring_dim] );

            for_less( voxel[blurring_dim], 0, sizes[blurring_dim] )
            {
                value = output[voxel[blurring_dim]];
                if( value < volume_min )
                    value = volume_min;
                else if( value > volume_max )
                    value = volume_max;

                set_volume_real_value( volume, voxel[0], voxel[1], voxel[2],
                                       voxel[3], voxel[4], value );
            }

            voxel[blurring_dim] = 0;
        }

        print( "Blurred Dimensions %d out of %d\n", blurring_dim+1, n_dims );
    }

    FREE( values );
    FREE( output );
}
