#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char      *input_volume_filename, *output_filename;
    char      *second_output;
    Volume    volume, label_volume, read_label_volume;
    int       sizes[N_DIMENSIONS], voxel[N_DIMENSIONS];
    int       true_label, test_label;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_string_argument( "", &second_output ) )
    {
        print( "Usage: %s  volume_file  output_file\n", argv[0] );
        return( 1 );
    }

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_BYTE );

    get_volume_sizes( volume, sizes );

    for_less( voxel[X], 1, sizes[X]-1 )
    for_less( voxel[Y], 1, sizes[Y]-1 )
    for_less( voxel[Z], 1, sizes[Z]-1 )
    {
        set_volume_label_data( label_volume, voxel, 1 );
    }

    if( save_label_volume( output_filename, NULL, label_volume ) != OK )
        return( 1 );

    read_label_volume = create_label_volume( volume, NC_BYTE );

    if( load_label_volume( output_filename, read_label_volume ) != OK )
        return( 1 );

    for_less( voxel[X], 0, sizes[X] )
    for_less( voxel[Y], 0, sizes[Y] )
    for_less( voxel[Z], 0, sizes[Z] )
    {
        true_label = get_volume_label_data( label_volume, voxel );
        test_label = get_volume_label_data( read_label_volume, voxel );

        if( true_label != test_label )
        {
            print( "%d %d %d  : %d != %d\n", voxel[X], voxel[Y], voxel[Z],
                   true_label, test_label );
            handle_internal_error( "error" );
        }
    }

    if( save_label_volume( second_output, output_filename, read_label_volume )
                               != OK )
        return( 1 );

    if( load_label_volume( second_output, read_label_volume ) != OK )
        return( 1 );

    for_less( voxel[X], 0, sizes[X] )
    for_less( voxel[Y], 0, sizes[Y] )
    for_less( voxel[Z], 0, sizes[Z] )
    {
        true_label = get_volume_label_data( label_volume, voxel );
        test_label = get_volume_label_data( read_label_volume, voxel );

        if( true_label != test_label )
        {
            print( "%d %d %d  : %d != %d\n", voxel[X], voxel[Y], voxel[Z],
                   true_label, test_label );
            handle_internal_error( "error" );
        }
    }

    return( 0 );
}
