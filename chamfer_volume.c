#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: dilate_volume input.mnc output.mnc  min max [6|26]\n\
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
    STRING               input_filename, output_filename;
    Real                 min_value, max_value;
    Volume               volume, label_volume;
    int                  n_neighs, n_changed, label;
    int                  range_changed[2][N_DIMENSIONS];
    Neighbour_types      connectivity;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &min_value ) ||
        !get_real_argument( 0.0, &max_value ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 26, &n_neighs );

    switch( n_neighs )
    {
    case 6:   connectivity = FOUR_NEIGHBOURS;  break;
    case 26:   connectivity = EIGHT_NEIGHBOURS;  break;
    default:  print_error( "# neighs must be 6 or 26.\n" );  return( 1 );
    }

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      NULL ) != OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_BYTE );

    modify_labels_in_range( volume, label_volume, 0, 0, 100,
                            min_value, max_value, range_changed );

    delete_volume( volume );

    label = 101;
    do
    {
        n_changed = dilate_voxels_3d( NULL, label_volume,
                                      1.0, (Real) (label-1),
                                      0.0, -1.0,
                                      0.0, 0.0,
                                      0.0, -1.0,
                                      (Real) label, connectivity,
                                      range_changed );
        print( "%d\n", n_changed );
        ++label;
    }
    while( n_changed > 0 && label <= 255 );

    modify_labels_in_range( label_volume, label_volume, 100, 100, 0,
                            0.0, -1.0, range_changed );

    label = 100;
    do
    {
        n_changed = dilate_voxels_3d( NULL, label_volume,
                                      (Real) (label+1), (Real) (label+1),
                                      0.0, -1.0,
                                      0.0, 0.0,
                                      0.0, -1.0,
                                      (Real) label, connectivity,
                                      range_changed );
        print( "%d\n", n_changed );
        --label;
    }
    while( n_changed > 0 && label >= 0 );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, label_volume, input_filename,
                                   "Chamfered\n",
                                   NULL );

    return( 0 );
}
