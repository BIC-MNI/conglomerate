#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    Volume             volume, new_volume;
    STRING             input_filename, output_filename;
    int                v[MAX_DIMENSIONS], dim, n_bins, v0, bin;
    int                sizes[MAX_DIMENSIONS];
    int                max_size, new_sizes[MAX_DIMENSIONS], **counts;
    int                max_count;
    STRING             *new_dim_names;
    Real               value, min_value, max_value;
    Real               separations[MAX_DIMENSIONS], bin_separation;
    General_transform  transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s input.mnc  colour_map.txt  output.mnc\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 50, &n_bins );
    (void) get_real_argument( 1.0, &bin_separation );

    if( input_volume( input_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    min_value = get_volume_real_min( volume );
    max_value = get_volume_real_max( volume );

    get_volume_sizes( volume, sizes );

    max_size = MAX3( sizes[0], sizes[1], sizes[2] );

    new_sizes[0] = 3;
    new_sizes[1] = max_size;
    new_sizes[2] = n_bins;

    ALLOC( new_dim_names, N_DIMENSIONS );

    new_dim_names[0] = MIxspace;
    new_dim_names[1] = MIyspace;
    new_dim_names[2] = MIzspace;

    new_volume =  create_volume( N_DIMENSIONS, new_dim_names,
                                 NC_FLOAT, FALSE, 0.0, 0.0 );
    set_volume_sizes( new_volume, new_sizes );
    alloc_volume_data( new_volume );

    separations[0] = 1.0;
    separations[1] = 1.0;
    separations[2] = bin_separation;
    set_volume_separations( new_volume, separations );

    ALLOC2D( counts, max_size, n_bins );
    max_count = 0;

    for_less( dim, 0, N_DIMENSIONS )
    {
        for_less( v0, 0, max_size )
        for_less( bin, 0, n_bins )
            counts[v0][bin] = 0;

        for_less( v[0], 0, sizes[0] )
        for_less( v[1], 0, sizes[1] )
        for_less( v[2], 0, sizes[2] )
        {
            value = get_volume_real_value( volume, v[0], v[1], v[2], 0, 0 );
            bin = (value - min_value) / (max_value - min_value) * n_bins;
            if( bin == n_bins )
                bin = n_bins-1;

            ++counts[v[dim]][bin];
            if( counts[v[dim]][bin] > max_count )
                max_count = counts[v[dim]][bin];
        }

        for_less( v0, 0, max_size )
        for_less( bin, 0, n_bins )
        {
            set_volume_real_value( new_volume, dim, v0, bin, 0, 0,
                                   (Real) counts[v0][bin] );
        }
    }

    print( "%g %d\n", (max_value - min_value) / (Real) n_bins, max_count );

    set_volume_voxel_range( new_volume, 0.0, (Real) max_count );
    set_volume_real_range( new_volume, 0.0, (Real) max_count );

    FREE2D( counts );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0,
                          new_volume, "Histogrammed\n",
                          (minc_output_options *) NULL );

    return( 0 );
}
