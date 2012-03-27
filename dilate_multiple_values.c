#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: dilate_volume input.mnc output.mnc  dilation_value\n\
            [6|26]  [n_dilations]  [mask.mnc min_mask max_mask]\n\
\n\
     Dilates all regions of value dilation_value, by n_dilations of 3X3X3,\n\
     (1 dilation by default).  You can specify 6 or 26 neighbours, default\n\
     being 26.  If the mask volume and range is specified, then only voxels\n\
     in the specified mask range will be dilated.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename, mask_filename;
    STRING               *dim_names;
    Real                 min_mask, max_mask, value_to_dilate;
    BOOLEAN              mask_volume_present;
    Volume               volume, mask_volume;
    int                  i, n_dilations, n_neighs, n_changed;
    int                  range_changed[2][N_DIMENSIONS];
    Neighbour_types      connectivity;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &value_to_dilate ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 26, &n_neighs );
    (void) get_int_argument( 1, &n_dilations );

    mask_volume_present = get_string_argument( "", &mask_filename );

    if( mask_volume_present &&
        (!get_real_argument( 0.0, &min_mask ) ||
         !get_real_argument( 0.0, &max_mask )) )
    {
        usage( argv[0] );
        return( 1 );
    }

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

    if( mask_volume_present )
    {
        dim_names = get_volume_dimension_names( volume );

        if( input_volume( mask_filename, 3, dim_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &mask_volume,
                          (minc_input_options *) NULL ) != OK )
            return( 1 );

        delete_dimension_names( volume, dim_names );

        if( !volumes_are_same_grid( mask_volume, volume ) )
        {
            print_error( "Mask volume must be on same grid as volume.\n" );
            return( 1 );
        }
    }
    else
    {
        mask_volume = NULL;
        min_mask = 0.0;
        max_mask = -1.0;
    }

    for_less( i, 0, n_dilations )
    {
        n_changed = dilate_voxels_3d( mask_volume, volume,
                                      value_to_dilate, value_to_dilate,
                                      0.0, -1.0,
                                      0.0, -1.0, min_mask, max_mask,
                                      value_to_dilate, connectivity,
                                      range_changed );
        print( "%d\n", n_changed );
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, input_filename,
                                   "Dilated\n",
                                   (minc_output_options *) NULL );

    return( 0 );
}
