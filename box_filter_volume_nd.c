#include  <volume_io.h>
#include  <bicpl.h>

private  void  box_filter_volume(
    Volume   volume,
    Real     filter_widths[] );

private  void  usage(
    STRING   executable )
{
    STRING   usage_str = "\n\
Usage: %s input.mnc output.mnc x_width [y_width [z_width [width4 [width5]]]]\n\
\n\
     Box filters a volume with the given WORLD coordinate widths,\n\n";

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
                      NC_BYTE, FALSE, 0.0, 255.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    n_dims = get_volume_n_dimensions( volume );

    for_less( dim, 0, n_dims )
    {
        if( !get_real_argument( 0.0, &filter_widths[dim] ) )
        {
            usage( argv[0] );
            return( 1 );
        }
    }

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

private  void  box_filter_1d_simple(
    int   size,
    Real  values[],
    Real  output[],
    Real  width )
{
    int   start, end, v;
    Real  half_width, current;

    half_width = width / 2.0;

    start = FLOOR( -half_width + 0.5 );
    end = FLOOR( half_width + 0.5 );

    current = 0.0;
    for_less( v, 0, MIN(end,size) )
        current += values[v];

    for_less( v, 0, size )
    {
        output[v] = current / width;
        if( end < size )
            current += values[end];
        ++end;

        if( start >= 0 )
            current -= values[start];
        ++start;
    }
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

    left_weight /= width;
    right_weight /= width;

    current = 0.0;

    start = FLOOR( -half_width + 0.5 );
    end = FLOOR( half_width + 0.5 );

    for_less( v, 0, MIN(end,size) )
        current += values[v] / width;
    if( end <= size-1 )
        current += left_weight * values[end];

    for_less( v, 0, size )
    {
        output[v] = current;
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
    int   v, dim, n_dims, blurring_dim, n;
    int   v0, v1, v2, v3, v4;
    int   sizes[MAX_DIMENSIONS], max_size;
    int   start[MAX_DIMENSIONS];
    int   end[MAX_DIMENSIONS];
    int   count[MAX_DIMENSIONS];
    int   count0, count1, count2, count3, count4;
    int   start0, start1, start2, start3, start4;
    int   end0, end1, end2, end3, end4;
    Real  *values, *output, volume_min, volume_max;
    BOOLEAN  simple_case;

    n_dims = get_volume_n_dimensions( volume );
    get_volume_sizes( volume, sizes );

    get_volume_voxel_range( volume, &volume_min, &volume_max );

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
        n = sizes[blurring_dim];

        count0 = count[0];
        count1 = count[1];
        count2 = count[2];
        count3 = count[3];
        count4 = count[4];

        start0 = start[0];
        start1 = start[1];
        start2 = start[2];
        start3 = start[3];
        start4 = start[4];

        end0 = end[0];
        end1 = end[1];
        end2 = end[2];
        end3 = end[3];
        end4 = end[4];

        simple_case = numerically_close(
                        FRACTION( filter_widths[blurring_dim] / 2.0 ), 0.5,
                        1.0e-6 );

        for_inclusive( v0, start0, end0 )
        for_inclusive( v1, start1, end1 )
        for_inclusive( v2, start2, end2 )
        for_inclusive( v3, start3, end3 )
        for_inclusive( v4, start4, end4 )
        {
            get_volume_voxel_hyperslab( volume, v0, v1, v2, v3, v4,
                                        count0, count1, count2, count3, count4,
                                        values );

            if( simple_case )
                box_filter_1d_simple( n, values, output,
                                      filter_widths[blurring_dim] );
            else
                box_filter_1d( n, values, output, filter_widths[blurring_dim] );

            for_less( v, 0, n )
            {
                if( output[v] < volume_min )
                    output[v] = volume_min;
                else if( output[v] > volume_max )
                    output[v] = volume_max;
            }

            set_volume_voxel_hyperslab( volume, v0, v1, v2, v3, v4,
                                        count0, count1, count2,
                                        count3, count4, output );
        }

        print( "Blurred Dimensions %d out of %d\n", blurring_dim+1, n_dims );
    }

    FREE( values );
    FREE( output );
}
