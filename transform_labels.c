#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  labels.mnc  like.mnc  output.mnc\n\
\n\
\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               labels_filename, like_filename, output_filename;
    int                  sizes[MAX_DIMENSIONS], int_label;
    int                  n_voxels, x, y, z;
    Real                 voxel2[N_DIMENSIONS];
    Volume               labels, new_labels, like_volume;
    Real                 separations[MAX_DIMENSIONS];
    Real                 xw, yw, zw, label;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &labels_filename ) ||
        !get_string_argument( "", &like_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( labels_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &labels,
                      NULL ) != OK )
        return( 1 );

    if( input_volume_header_only( like_filename, 3, NULL, &like_volume, NULL
                                              ) != OK )
        return( 1 );

    new_labels = copy_volume_definition( like_volume, NC_BYTE, FALSE, 0.0, 0.0);

    get_volume_sizes( new_labels, sizes );

    n_voxels = 0;

    initialize_progress_report( &progress, FALSE, sizes[X], "Transforming" );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            voxel2[X] = (Real) x;
            voxel2[Y] = (Real) y;
            voxel2[Z] = (Real) z;
            convert_voxel_to_world( new_labels, voxel2, &xw, &yw, &zw );

            evaluate_volume_in_world( labels, xw, yw, zw, -1, TRUE, 0.0,
                                      &label, NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, NULL, NULL );

            int_label = ROUND( label );

            if( int_label != 0 )
                ++n_voxels;

            set_volume_voxel_value( new_labels, x, y, z, 0, 0, int_label );
        }

        update_progress_report( &progress, x + 1 );
    }

    terminate_progress_report( &progress );

    get_volume_separations( new_labels, separations );

    print( "N voxels: %d\n", n_voxels );
    print( "Volume  : %g\n", (Real) n_voxels * separations[0] * separations[1]
                             * separations[2] );

    return( 0 );
}
