#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING   usage_str = "\n\
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
    Volume     volume, new_volume;
    Status     status;
    Real       voxel_filter_widths[N_DIMENSIONS];
    Real       filter_widths[N_DIMENSIONS];
    Real       separations[MAX_DIMENSIONS];
    nc_type    data_type;
    BOOLEAN    world_space, byte_flag;
    STRING     input_filename, output_filename, history, dummy;
    STRING     space_text;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 3.0, &filter_widths[X] );
    (void) get_real_argument( 3.0, &filter_widths[Y] );
    (void) get_real_argument( 3.0, &filter_widths[Z] );

    world_space = get_string_argument( "", &space_text ) &&
                  equal_strings( space_text, "world" );
    byte_flag = get_string_argument( "", &dummy );

    if( byte_flag )
        data_type = NC_BYTE;
    else
        data_type = NC_UNSPECIFIED;

    status = input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) ;

    if( status != OK )
        return( 1 );

    reorder_xyz_to_voxel( volume, filter_widths, voxel_filter_widths );

    if( world_space )
    {
        get_volume_separations( volume, separations );
        for_less( c, 0, N_DIMENSIONS )
            voxel_filter_widths[c] /= ABS( separations[c] );
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
