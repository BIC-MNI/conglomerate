#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  dilate_values_in_row(
    int    n,
    Real   values[],
    Real   out_values[],
    Real   min_inside,
    Real   max_inside,
    Real   min_outside,
    Real   max_outside,
    int    n_dilations,
    int    max_buffer[] );

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s input.mnc output.mnc  min_region max_region \n\
            [n_dilations]  [min_outside] [max_outside]\n\
\n\
     Dilates all regions within the specified range in a 3X3X3 kernel,\n\
     (1 dilation by default).  The regions dilated into are specified by \n\
     the min_outside and max_outside, defaulting to 0 and 0.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename;
    Real                 min_inside, max_inside;
    Real                 min_outside, max_outside;
    Volume               volume;
    int                  n_dilations, dim;
    Real                 *values, *out_values;
    int                  *max_buffer;
    int                  sizes[N_DIMENSIONS];
    int                  tmp_voxel[N_DIMENSIONS];
    int                  min_voxel[N_DIMENSIONS];
    int                  max_voxel[N_DIMENSIONS];
    int                  n_voxels[N_DIMENSIONS];
    int                  voxel[N_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &min_inside ) ||
        !get_real_argument( 0.0, &max_inside ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1, &n_dilations );
    (void) get_real_argument( 0.0, &min_outside );
    (void) get_real_argument( 0.0, &max_outside );

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    ALLOC( values, MAX3(sizes[0],sizes[1],sizes[2]) );
    ALLOC( out_values, MAX3(sizes[0],sizes[1],sizes[2]) );
    ALLOC( max_buffer, MAX3(sizes[0],sizes[1],sizes[2]) );

    if( n_dilations < 0 )
        n_dilations = MAX3(sizes[0],sizes[1],sizes[2]);

    for_less( dim, 0, N_DIMENSIONS )
    {
        min_voxel[0] = 0;
        max_voxel[0] = sizes[0]-1;
        min_voxel[1] = 0;
        max_voxel[1] = sizes[1]-1;
        min_voxel[2] = 0;
        max_voxel[2] = sizes[2]-1;

        n_voxels[0] = 1;
        n_voxels[1] = 1;
        n_voxels[2] = 1;
        n_voxels[dim] = sizes[dim];

        max_voxel[dim] = 0;

        for_inclusive( voxel[0], min_voxel[0], max_voxel[0] )
        for_inclusive( voxel[1], min_voxel[1], max_voxel[1] )
        for_inclusive( voxel[2], min_voxel[2], max_voxel[2] )
        {
            get_volume_value_hyperslab_3d( volume, voxel[0], voxel[1], voxel[2],
                                           n_voxels[0], n_voxels[1],
                                           n_voxels[2], values );

            dilate_values_in_row( sizes[dim], values, out_values,
                                  min_inside, max_inside,
                                  min_outside, max_outside,
                                  n_dilations, max_buffer );

            tmp_voxel[0] = voxel[0];
            tmp_voxel[1] = voxel[1];
            tmp_voxel[2] = voxel[2];
            for_less( tmp_voxel[dim], 0, sizes[dim] )
                set_volume_real_value( volume,
                                       tmp_voxel[0], tmp_voxel[1], tmp_voxel[2],
                                       0, 0, out_values[tmp_voxel[dim]] );
        }
    }

    FREE( values );
    FREE( max_buffer );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, input_filename,
                                   "dilate_all_labels\n",
                                   NULL );

    return( 0 );
}

private  void  dilate_values_in_row(
    int    n,
    Real   values[],
    Real   out_values[],
    Real   min_inside,
    Real   max_inside,
    Real   min_outside,
    Real   max_outside,
    int    n_dilations,
    int    max_buffer[] )
{
    int    i, current, n_maxes, start, end, n_init;

    n_init = MIN( n, n_dilations+1 );

    n_maxes = 0;

    for_less( i, 0, n_init )
    {
        while( n_maxes > 0 && values[i] >= values[max_buffer[n_maxes-1]] )
            --n_maxes;

        max_buffer[n_maxes] = i;
        ++n_maxes;
    }

    end = n_dilations;
    start = -n_dilations;

    for_less( current, 0, n )
    {
        out_values[current] = values[max_buffer[0]];

        ++end;
        if( end < n )
        {
            while( n_maxes > 0 && values[end] >= values[max_buffer[n_maxes-1]] )
                --n_maxes;

            max_buffer[n_maxes] = end;
            ++n_maxes;
        }

        if( start >= 0 && start == max_buffer[0] )
        {
            --n_maxes;
            for_less( i, 0, n_maxes )
                max_buffer[i] = max_buffer[i+1];
        }

        ++start;
    }
}
