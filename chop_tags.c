#include  <volume_io/internal_volume_io.h>
#include  <special_geometry.h>

private  void  usage(
    STRING  executable )
{
    static  STRING  usage_str = "\n\
Usage: %s  input.tag  output.tag  [x|y|z min max] [x|y|z min max] ...\n\
\n\
     Clips tags between any number of x, y, or z limits.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_tags_filename, output_tags_filename;
    STRING               axis_name;
    int                  tag, axis;
    Real                 min_pos, max_pos;
    int                  n_volumes, n_tag_points, *structure_ids, *patient_ids;
    Real                 **tags1, **tags2, *weights;
    STRING               *labels;
    int                  n_new_tags, *new_structure_ids, *new_patient_ids;
    Real                 **new_tags1, **new_tags2, *new_weights;
    STRING               *new_labels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_tags_filename ) ||
        !get_string_argument( "", &output_tags_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_tag_file( input_tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, &weights, &structure_ids,
                        &patient_ids, &labels ) != OK )
        return( 1 );

    new_structure_ids = NULL;
    new_patient_ids = NULL;
    new_tags1 = NULL;
    new_tags2 = NULL;
    new_weights = NULL;
    new_labels = NULL;

    while( get_string_argument( NULL, &axis_name ) &&
           get_real_argument( 0.0, &min_pos ) &&
           get_real_argument( 0.0, &max_pos ) )
    {
        if( string_length( axis_name ) != 1 ||
            axis_name[0] < 'x' || axis_name[0] > 'z' ||
            min_pos > max_pos )
        {
            usage( argv[0] );
        }

        axis = (int) (axis_name[0] - 'x');

        n_new_tags = 0;

        for_less( tag, 0, n_tag_points )
        {
            if( tags1[tag][axis] >= min_pos && tags1[tag][axis] <= max_pos )
            {
                SET_ARRAY_SIZE( new_tags1, n_new_tags, n_new_tags+1, 10 );
                ALLOC( new_tags1[n_new_tags], 3 );

                SET_ARRAY_SIZE( new_weights, n_new_tags, n_new_tags+1, 10 );
                SET_ARRAY_SIZE( new_structure_ids, n_new_tags, n_new_tags+1,10);
                SET_ARRAY_SIZE( new_patient_ids, n_new_tags, n_new_tags+1, 10 );

                SET_ARRAY_SIZE( new_labels, n_new_tags, n_new_tags+1, 10 );

                /*--- copy from the input tags to the new tags */

                new_tags1[n_new_tags][0] = tags1[tag][0];
                new_tags1[n_new_tags][1] = tags1[tag][1];
                new_tags1[n_new_tags][2] = tags1[tag][2];

                if( n_volumes == 2 )
                {
                    SET_ARRAY_SIZE( new_tags2, n_new_tags, n_new_tags+1, 10 );
                    ALLOC( new_tags2[n_new_tags], 3 );

                    new_tags2[n_new_tags][0] = tags2[tag][0];
                    new_tags2[n_new_tags][1] = tags2[tag][1];
                    new_tags2[n_new_tags][2] = tags2[tag][2];
                }

                new_weights[n_new_tags] = weights[tag];
                new_structure_ids[n_new_tags] = structure_ids[tag];
                new_patient_ids[n_new_tags] = patient_ids[tag];
                new_labels[n_new_tags] = create_string( labels[tag] );

                /*--- increment the number of new tags */

                ++n_new_tags;
            }
        }

        free_tag_points( n_volumes, n_tag_points, tags1, tags2,
                         weights, structure_ids, patient_ids, labels );

        n_tag_points = n_new_tags;
        tags1 = new_tags1;
        tags2 = new_tags2;
        weights = new_weights;
        structure_ids = new_structure_ids;
        patient_ids = new_patient_ids;
        labels = new_labels;
    }

    if( output_tag_file( output_tags_filename, "Chopped off some tags",
                         n_volumes, n_tag_points, tags1, tags2,
                         weights, structure_ids,
                         patient_ids, labels ) != OK )
        return( 1 );

    free_tag_points( n_volumes, n_tag_points, tags1, tags2,
                     weights, structure_ids, patient_ids, labels );

    return( 0 );
}
