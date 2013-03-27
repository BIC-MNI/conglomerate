#include  <mni.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Status               status;
    char                 *input_filename, *input_lmk_file;
    char                 *output_tag_filename;
    VIO_Volume               volume;
    volume_input_struct  volume_input;
    int                  n_objects;
    object_struct        **object_list;
    int                  i;
    marker_struct        *marker;
    int                  n_tags;
    VIO_Real                 **tags, *weights;
    int                  *structure_ids, *patient_ids;
    char                 **labels;
    FILE                 *file;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &input_lmk_file ) ||
        !get_string_argument( "", &output_tag_filename ) )
    {
        print( "%s  example_volume  input_lmk_file  output_tag_file\n", argv[0] );
        return( 1 );
    }

    status = start_volume_input( input_filename, 3, XYZ_dimension_names,
                                 NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                                 TRUE, &volume, (minc_input_options *) NULL,
                                 &volume_input );

    status = input_objects_any_format( volume, input_lmk_file,
                                       GREEN, 1.0, BOX_MARKER,
                                       &n_objects, &object_list );

    ALLOC2D( tags, n_objects, N_DIMENSIONS );
    ALLOC( weights, n_objects );
    ALLOC( structure_ids, n_objects );
    ALLOC( patient_ids, n_objects );
    ALLOC2D( labels, n_objects, MAX_STRING_LENGTH );

    n_tags = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );

            tags[n_tags][X] = Point_x(marker->position);
            tags[n_tags][Y] = Point_y(marker->position);
            tags[n_tags][Z] = Point_z(marker->position);

            weights[n_tags] = marker->size;
            structure_ids[n_tags] = marker->structure_id;
            patient_ids[n_tags] = marker->patient_id;
            (void) strcpy( labels[n_tags], marker->label );
            ++n_tags;
        }

    }

    delete_object_list( n_objects, object_list );


    status = open_file_with_default_suffix( output_tag_filename,
                              get_default_tag_file_suffix(), WRITE_FILE,
                              ASCII_FORMAT, &file );

    if( status == VIO_OK )
        status = output_tag_points( file, (char *) NULL, 1, n_tags,
                                    tags, (VIO_Real **) NULL, weights,
                                    structure_ids, patient_ids, labels );

    if( status == VIO_OK )
        status = close_file( file );

    return( status != VIO_OK );
}
