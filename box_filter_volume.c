#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR   usage_str = "\n\
Usage: %s input.mnc output.mnc x_width y_width z_width\n\
           [world|voxel] [byte]\n\
\n\
     Box filters a volume with the given voxel widths, or, if [world] specified,\n\
     then in mm widths.  If byte is specified, then a byte volume is created.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    int        c;
    VIO_Volume     volume, new_volume;
    VIO_Status     status;
    VIO_Real       voxel_filter_widths[VIO_N_DIMENSIONS];
    VIO_Real       filter_widths[VIO_N_DIMENSIONS];
    VIO_Real       separations[VIO_MAX_DIMENSIONS];
    nc_type     data_type;
    VIO_BOOL    world_space, byte_flag;
    VIO_STR     input_filename, output_filename, history, dummy;
    VIO_STR     space_text;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 3.0, &filter_widths[VIO_X] );
    (void) get_real_argument( 3.0, &filter_widths[VIO_Y] );
    (void) get_real_argument( 3.0, &filter_widths[VIO_Z] );

    world_space = get_string_argument( "", &space_text ) &&
                  equal_strings( space_text, "world" );
    byte_flag = get_string_argument( "", &dummy );

    if( byte_flag )
        data_type = NC_BYTE;
    else
        data_type = NC_UNSPECIFIED;

/*
    save_threshold = get_n_bytes_cache_threshold();
    save_max_bytes = get_default_max_bytes_in_cache();

    set_n_bytes_cache_threshold( 0 );
    set_default_max_bytes_in_cache( 0 );
    block_sizes[0] = 1;
    block_sizes[1] = -1;
    block_sizes[2] = -1;
    set_default_cache_block_sizes( block_sizes );
*/

    status = input_volume( input_filename, 3, File_order_dimension_names,
                      NC_BYTE, FALSE, 0.0, 255.0,
                      TRUE, &volume, (minc_input_options *) NULL ) ;

    if( status != VIO_OK )
        return( 1 );

/*
    set_n_bytes_cache_threshold( save_threshold );
    set_default_max_bytes_in_cache( save_max_bytes );
    for_less( dim, 0, VIO_MAX_DIMENSIONS )
        block_sizes[dim] = 8;
    set_default_cache_block_sizes( block_sizes );
*/

    reorder_xyz_to_voxel( volume, filter_widths, voxel_filter_widths );

    if( world_space )
    {
        get_volume_separations( volume, separations );
        for_less( c, 0, VIO_N_DIMENSIONS )
            voxel_filter_widths[c] /= VIO_FABS( separations[c] );
    }

    new_volume = create_box_filtered_volume( volume, data_type, FALSE,
                                             0.0, 0.0,
                                             voxel_filter_widths[0],
                                             voxel_filter_widths[1],
                                             voxel_filter_widths[2] );

    history = "Box filtered\n";

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0,
                          new_volume, history, (minc_output_options *) NULL );


    return( 0 );
}
