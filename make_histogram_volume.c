#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <images.h>

#define  TWO_D

int  main(
    int   argc,
    char  *argv[] )
{
    Volume             volume, new_volume;
    STRING             input_filename, output_filename;
    int                v[MAX_DIMENSIONS], n_bins, bin;
    int                vv[MAX_DIMENSIONS], dim1, dim2, dim3;
    int                sizes[MAX_DIMENSIONS], n_dims;
    int                new_sizes[MAX_DIMENSIONS];
    int                n_slices, min_v1, max_v1, min_v2, max_v2, **total_counts;
    int                v1;
    STRING             *new_dim_names;
    Real               value, min_value, max_value;
    Real               separations[MAX_DIMENSIONS], bin_separation;
    progress_struct    progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( NULL, &dim1 ) ||
        !get_int_argument( NULL, &dim2 ) )
    {
        print( "Usage: %s input.mnc  output.mnc  dim1 dim2\n", argv[0] );
        return( 1 );
    }

    n_dims = 1;
    if( dim1 < 0 || dim1 >= 3 )
    {
        print( "Usage: %s input.mnc  colour_map.txt  output.mnc\n", argv[0] );
        return( 1 );
    }

    if( dim2 >= 0 && dim2 < 3 )
        ++n_dims;
    else
        dim2 = (dim1 + 1) % N_DIMENSIONS;

    dim3 = 3 - dim1 - dim2;

    (void) get_int_argument( 50, &n_bins );
    (void) get_real_argument( 1.0, &bin_separation );
    (void) get_int_argument( 0, &n_slices );

    if( input_volume( input_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    min_value = get_volume_real_min( volume );
    max_value = get_volume_real_max( volume );

    get_volume_sizes( volume, sizes );
    get_volume_separations( volume, separations );

    if( n_dims == 2 )
    {
        ALLOC2D( total_counts, sizes[dim1], sizes[dim2] );
        new_sizes[dim1] = sizes[dim1];
        new_sizes[dim2] = sizes[dim2];
        new_sizes[dim3] = n_bins;
        separations[dim3] = bin_separation;
    }
    else
    {
        ALLOC2D( total_counts, 1, sizes[dim1] );
        new_sizes[0] = 1;
        new_sizes[1] = sizes[dim1];
        new_sizes[2] = n_bins;
        separations[0] = 1.0;
        separations[1] = 1.0;
        separations[2] = bin_separation;
    }

    new_dim_names = get_volume_dimension_names( volume );

    new_volume =  create_volume( N_DIMENSIONS, new_dim_names,
                                 NC_FLOAT, FALSE, 0.0, 0.0 );
    set_volume_sizes( new_volume, new_sizes );
    alloc_volume_data( new_volume );
    set_volume_separations( new_volume, separations );

    v[0] = 0;
    v[1] = 0;
    v[2] = 0;

    if( n_dims == 2 )
    {
        for_less( v[dim1], 0, sizes[dim1] )
        for_less( v[dim2], 0, sizes[dim2] )
        {
            total_counts[v[dim1]][v[dim2]] = 0;
            for_less( v[dim3], 0, n_bins )
                set_volume_real_value( new_volume, v[0], v[1], v[2], 0, 0, 0.0);
        }
    }
    else
    {
        for_less( v1, 0, sizes[dim1] )
        {
            total_counts[0][v1] = 0;
            for_less( bin, 0, n_bins )
                set_volume_real_value( new_volume, 0, v1, bin, 0, 0, 0.0);
        }
    }

    initialize_progress_report( &progress, FALSE, sizes[0] * sizes[1],
                                "Histogramming" );

    for_less( v[0], 0, sizes[0] )
    for_less( v[1], 0, sizes[1] )
    {
    for_less( v[2], 0, sizes[2] )
    {
        value = get_volume_real_value( volume, v[0], v[1], v[2], 0, 0 );

        bin = (value - min_value) / (max_value - min_value) * n_bins;

        if( bin == n_bins )
            bin = n_bins-1;

        min_v1 = MAX( 0, v[dim1] - n_slices );
        max_v1 = MIN( sizes[dim1]-1, v[dim1] + n_slices );

        if( n_dims == 1 )
        {
            for_inclusive( v1, min_v1, max_v1 )
            {
                value = get_volume_real_value( new_volume, 0, v1, bin, 0, 0 );
                set_volume_real_value( new_volume, 0, v1, bin, 0, 0,
                                       value + 1.0 );
                ++total_counts[0][v1];
            }
        }
        else
        {
            vv[dim3] = bin;

            min_v2 = MAX( 0, v[dim2] - n_slices );
            max_v2 = MIN( sizes[dim2]-1, v[dim2] + n_slices );

            for_inclusive( vv[dim1], min_v1, max_v1 )
            for_inclusive( vv[dim2], min_v2, max_v2 )
            {
                value = get_volume_real_value( new_volume, vv[0], vv[1], vv[2],
                                               0, 0 );
                set_volume_real_value( new_volume, vv[0], vv[1], vv[2], 0, 0,
                                       value + 1.0 );
                ++total_counts[vv[dim1]][vv[dim2]];
            }
        }
    }
    update_progress_report( &progress, v[0] * sizes[1] + v[1] + 1 );
    }

    terminate_progress_report( &progress );

    if( n_dims == 1 )
    {
        for_less( v1, 0, sizes[dim1] )
        for_less( bin, 0, n_bins )
        {
            value = get_volume_real_value( new_volume, 0, v1, bin, 0, 0 );
            set_volume_real_value( new_volume, 0, v1, bin, 0, 0,
                                10000.0 * value / (Real) total_counts[0][v1] );
        }
    }
    else
    {
        for_less( v[dim1], 0, sizes[dim1] )
        for_less( v[dim2], 0, sizes[dim2] )
        for_less( v[dim3], 0, n_bins )
        {
            value = get_volume_real_value( new_volume, v[0], v[1], v[2], 0, 0 );
            set_volume_real_value( new_volume, v[0], v[1], v[2], 0, 0,
                    10000.0 * value / (Real) total_counts[v[dim1]][v[dim2]] );
        }
    }

    set_volume_voxel_range( new_volume, 0.0, 10000.0 );
    set_volume_real_range( new_volume, 0.0, 10000.0 );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0,
                          new_volume, "Histogrammed\n",
                          (minc_output_options *) NULL );

    return( 0 );
}
