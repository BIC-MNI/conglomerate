#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  example.mnc  input.tags output.tags\n\
\n\
     Flips tags about the left-right centre of the volume.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename, input_tags, output_tags;
    VIO_Volume               volume;
    VIO_Real                 voxel[VIO_N_DIMENSIONS];
    int                  i, sizes[VIO_N_DIMENSIONS];
    int                  n_volumes, n_tag_points;
    VIO_Real                 **tags_volume1, **tags_volume2, *weights;
    int                  *structure_ids, *patient_ids;
    volume_input_struct  volume_input;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &input_tags ) ||
        !get_string_argument( "", &output_tags ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( start_volume_input( volume_filename, 3, XYZ_dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != OK )
    {
        return( 1 );
    }

    if( input_tag_file( input_tags, &n_volumes, &n_tag_points,

                        &tags_volume1, &tags_volume2, &weights,
                        &structure_ids, &patient_ids, NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    for_less( i, 0, n_tag_points )
    {
        convert_world_to_voxel( volume, tags_volume1[i][X],
                                        tags_volume1[i][Y],
                                        tags_volume1[i][Z], voxel );
        voxel[X] = (VIO_Real) sizes[X] - 1.0 - voxel[X];

        convert_voxel_to_world( volume, voxel, &tags_volume1[i][X],
                                               &tags_volume1[i][Y],
                                               &tags_volume1[i][Z] );
    }

    (void) output_tag_file( output_tags, (char *) NULL,
                            n_volumes, n_tag_points,
                            tags_volume1, tags_volume2, weights,
                            structure_ids, patient_ids, NULL );

    return( 0 );
}
