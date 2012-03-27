#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.mnc  output.mnc  min_threshold  max_threshold\n\
           [6|26]\n\
\n\
     Creates a label volume where each connected component has\n\
     a distinct label number.\n\
     The connectivity is specified by the last argument as\n\
     either 6-neighbour or 26-neighbour.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    Real             min_threshold, max_threshold;
    int              sizes[N_DIMENSIONS], v[MAX_DIMENSIONS];
    int              range_changed[2][N_DIMENSIONS];
    int              current_label, num_labels, n_neighbours;
    STRING           volume_filename, output_filename;
    Volume           volume, label_volume;
    progress_struct  progress;
    Neighbour_types  connectivity;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &min_threshold ) ||
        !get_real_argument( 0.0, &max_threshold ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 26, &n_neighbours );
    if( n_neighbours == 6 )
        connectivity = FOUR_NEIGHBOURS;
    else if( n_neighbours == 26 )
        connectivity = EIGHT_NEIGHBOURS;
    else
    {
        print( "Connectivity specified must be either 6 or 26.\n" );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_UNSPECIFIED );

    num_labels = ROUND( get_volume_voxel_max( label_volume ) );

    get_volume_sizes( volume, sizes );

    current_label = 1;

    initialize_progress_report( &progress, FALSE, sizes[0] * sizes[1],
                                "Filling Regions" );

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        if( get_volume_label_data( label_volume, v ) == 0 )
        {
            if( fill_connected_voxels( volume, label_volume, connectivity,
                                       v, 0, 0, current_label,
                                       min_threshold, max_threshold,
                                       range_changed ) )
            {
                ++current_label;
                if( current_label >= num_labels )
                    current_label = 1;
            }
        }

        if( v[2] == sizes[2]-1 )
            update_progress_report( &progress, v[0] * sizes[1] + v[1] + 1 );

    END_ALL_VOXELS

    terminate_progress_report( &progress );

    print( "Created %d regions.\n", current_label-1 );

    (void) output_modified_volume( output_filename,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          label_volume, volume_filename,
                          "cluster_volume",
                          (minc_output_options *) NULL );


    delete_volume( volume );
    delete_volume( label_volume );

    return( 0 );
}
