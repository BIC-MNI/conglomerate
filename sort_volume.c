#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  avg.mnc input.mnc  output.mnc\n\
\n\
\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename, avg_filename;
    int                  v0, v1, v2, sizes[MAX_DIMENSIONS];
    int                  w0, w1, w2, index;
    int                  i, n_bins, *counts, count, sum;
    Volume               volume, avg, new_volume;
    Real                 min_voxel, max_voxel, voxel;
    int                  int_min_voxel, int_max_voxel;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &avg_filename ) ||
        !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_volume( avg_filename, 3, get_volume_dimension_names(volume),
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &avg, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( !volumes_are_same_grid( volume, avg ) )
    {
        print_error( "Volumes are not the same grid.\n" );
        return( 1 );
    }

    get_volume_sizes( volume, sizes );

    new_volume = copy_volume_definition( volume, NC_UNSPECIFIED, FALSE,
                                         0.0, 0.0 );

    get_volume_voxel_range( avg, &min_voxel, &max_voxel );

    int_min_voxel = ROUND( min_voxel );
    int_max_voxel = ROUND( max_voxel );

    n_bins = int_max_voxel - int_min_voxel + 1;
    ALLOC( counts, n_bins );

    for_less( i, 0, n_bins )
        counts[i] = 0;

    for_less( v0, 0, sizes[0] )
    {
        for_less( v1, 0, sizes[1] )
        {
            for_less( v2, 0, sizes[2] )
            {
                voxel = get_volume_voxel_value( avg, v0, v1, v2, 0, 0 );
                ++counts[ROUND(voxel)-int_min_voxel];
            }
        }
    }

    print( "Counted\n" );

    sum = 0;
    for_less( i, 0, n_bins )
    {
        count = counts[i];
        counts[i] = sum;
        sum += count;
    }

    for_less( v0, 0, sizes[0] )
    {
        for_less( v1, 0, sizes[1] )
        {
            for_less( v2, 0, sizes[2] )
            {
                voxel = get_volume_voxel_value( avg, v0, v1, v2, 0, 0 );
                index = counts[ROUND(voxel)-int_min_voxel];
                ++counts[ROUND(voxel)-int_min_voxel];

                w2 = index % sizes[2];
                index /= sizes[2];
                w1 = index % sizes[1];
                index /= sizes[1];
                w0 = index;

                voxel = get_volume_voxel_value( volume, v0, v1, v2, 0, 0 );
                set_volume_voxel_value( new_volume, w0, w1, w2, 0, 0, voxel );
            }
        }
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, new_volume, input_filename,
                                   "Reordered\n", NULL );

    return( 0 );
}
