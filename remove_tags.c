#include  <mni.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Status               status;
    char                 *input_volume_filename, *input_tag_filename;
    char                 *output_tag_filename;
    VIO_Real                 min_threshold, max_threshold;
    VIO_Volume               volume;
    VIO_Real                 value;
    int                  i, c, ind[MAX_DIMENSIONS];
    VIO_Real                 voxel[MAX_DIMENSIONS];
    int                  n_tags, n_volumes;
    VIO_Real                 **tags1, **tags2, *weights;
    int                  *structure_ids, *patient_ids;
    char                 **labels;
    int                  new_n_tags;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &input_tag_filename ) ||
        !get_real_argument( 0.0, &min_threshold ) ||
        !get_real_argument( 0.0, &max_threshold ) ||
        !get_string_argument( "", &output_tag_filename ) )
    {
        print( "Usage:  %s  input_volume input_tags min_threshold max_threshold output_tags.\n\n",
               argv[0] );
        print( "Example:   %s  patient1.mnc  tag_points.tag  105.5 256  new_tag_points.tag\n",
               argv[0] );
        return( 1 );
    }

    status = input_volume( input_volume_filename, 3, XYZ_dimension_names,
                           NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                           TRUE, &volume, (minc_input_options *) NULL );

    if( status != VIO_OK )
        return( 1 );

    status = input_tag_file( input_tag_filename, &n_volumes,
                             &n_tags, &tags1, &tags2,
                             &weights, &structure_ids, &patient_ids,
                             &labels );

    if( status != VIO_OK )
        return( 1 );

    new_n_tags = 0;

    for_less( i, 0, n_tags )
    {
        convert_world_to_voxel( volume,
                                tags1[i][X], tags1[i][Y], tags1[i][Z],
                                voxel );

        for_less( c, 0, N_DIMENSIONS )
            ind[c] = ROUND( voxel[c] );

        if( int_voxel_is_within_volume( volume, ind ) )
        {
            GET_VALUE_3D( value, volume, ind[X], ind[Y], ind[Z] );
            if( value >= min_threshold && value <= max_threshold )
            {
                if( new_n_tags != i )
                {
                    tags1[new_n_tags][X] = tags1[i][X];
                    tags1[new_n_tags][Y] = tags1[i][Y];
                    tags1[new_n_tags][Z] = tags1[i][Z];
                    if( weights != (VIO_Real *) NULL )
                        weights[new_n_tags] = weights[i];
                    if( structure_ids != (int *) NULL )
                        structure_ids[new_n_tags] = structure_ids[i];
                    if( patient_ids != (int *) NULL )
                        patient_ids[new_n_tags] = patient_ids[i];
                    if( labels != (char **) NULL )
                    {
                        REALLOC( labels[new_n_tags], strlen(labels[i])+1 );
                        (void) strcpy( labels[new_n_tags], labels[i] );
                    }
                }

                ++new_n_tags;
            }
        }
    }

    status = output_tag_file( output_tag_filename, (char *) NULL,
                              n_volumes, new_n_tags, tags1, tags2,
                              weights, structure_ids, patient_ids, labels );

    free_tag_points( n_volumes, n_tags, tags1, tags2, weights, structure_ids,
                     patient_ids, labels );

    return( status != VIO_OK );
}
