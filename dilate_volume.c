#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    char  executable[] )
{
    char  usage_str[] = "\n\
Usage: dilate_volume input.mnc output.mnc  dilation_value\n\
            [6|26]  [n_dilations]\n\
\n\
     Dilates all regions of value dilation_value, by n_dilations of 3X3X3,\n\
     (1 dilation by default).  You can specify 6 or 26 neighbours, default\n\
     being 26.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    Volume               volume;
    int                  i, n_dilations, n_neighs, value_to_dilate;
    Neighbour_types      connectivity;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_int_argument( 0, &value_to_dilate ) )
    {
        print( "%s input.mnc output.mnc  dilation_value\n", argv[0] );
        print( "      [6|26]  [n_dilations]\n" );
        return( 1 );
    }

    (void) get_int_argument( 8, &n_neighs );
    (void) get_int_argument( 1, &n_dilations );

    switch( n_neighs )
    {
    case 6:   connectivity = FOUR_NEIGHBOURS;  break;
    case 26:   connectivity = EIGHT_NEIGHBOURS;  break;
    default:  print_error( "# neighs must be 6 or 26.\n" );  return( 1 );
    }

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      (minc_input_options *) NULL ) != OK )
        return( 1 );

    for_less( i, 0, n_dilations )
    {
        dilate_labeled_voxels_3d( NULL, volume,
                                  0, -1,
                                  0.0, -1.0,
                                  0, -1, 0.0, -1.0,
                                  value_to_dilate, connectivity );
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, input_filename, "Dilated",
                                   (minc_output_options *) NULL );

    return( 0 );
}
