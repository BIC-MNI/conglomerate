#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s input.mnc output.mnc  [max_dilations|-1] [6|26] [min_outside max_outside]\n\
\n\
     Dilates all regions of volume until there are no values in the range\n\
     (min_outside, max_outside), which defaults to 0 0\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename;
    Volume               volume;
    int                  i, n_neighs, n_changed, iter, max_dilations;
    int                  range_changed[2][N_DIMENSIONS];
    int                  *counts, *labels, label, n_labels;
    int                  v0, v1, v2, v3, v4, n_done;
    Real                 min_volume, max_volume;
    Real                 min_outside, max_outside, value;
    BOOLEAN              *done;
    Neighbour_types      connectivity;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -1, &max_dilations );
    (void) get_int_argument( 26, &n_neighs );
    (void) get_real_argument( 0.0, &min_outside );
    (void) get_real_argument( 0.0, &max_outside );

    switch( n_neighs )
    {
    case 6:   connectivity = FOUR_NEIGHBOURS;  break;
    case 26:   connectivity = EIGHT_NEIGHBOURS;  break;
    default:  print_error( "# neighs must be 6 or 26.\n" );  return( 1 );
    }

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_real_range( volume, &min_volume, &max_volume );

    n_labels = ROUND( max_volume ) - ROUND( min_volume ) + 1;

    ALLOC( counts, n_labels );

    for_less( i, 0, n_labels )
        counts[i] = 0;

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )
        value = get_volume_real_value( volume, v0, v1, v2, v3, v4 );
        ++counts[ROUND(value) - ROUND(min_volume)];
    END_ALL_VOXELS

    ALLOC( labels, n_labels );
    n_labels = 0;
    for_inclusive( i, ROUND(min_volume), ROUND(max_volume) )
    {
        if( i != 0 && counts[i-ROUND(min_volume)] > 0 )
        {
            labels[n_labels] = i;
            ++n_labels;
        }
    }

    n_done = 0;
    ALLOC( done, n_labels );
    for_less( label, 0, n_labels )
        done[label] = FALSE;

    iter = 0;

    while( (max_dilations < 0 || iter < max_dilations) && n_done < n_labels )
    {
        ++iter;

        for_less( i, 0, n_labels )
        {
            if( done[i] )
                continue;

            label = labels[i];

            n_changed = dilate_voxels_3d( NULL, volume,
                                          (Real) label,
                                          (Real) label,
                                          0.0, -1.0,
                                          min_outside, max_outside,
                                          0.0, -1.0,
                                          (Real) label, connectivity,
                                          range_changed );

            print( "Iter: %d  Label %d:  %d\n", iter, label, n_changed );

            if( n_changed == 0 )
            {
                done[i] = TRUE;
                ++n_done;
            }
        }
    }

    FREE( done );
    FREE( labels );
    FREE( counts );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, input_filename,
                                   "dilate_volume_completely\n",
                                   NULL );

    return( 0 );
}
