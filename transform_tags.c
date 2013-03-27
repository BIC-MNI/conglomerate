#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input.tag  input.xfm  [output.tag]  [invert]\n\
\n\
     Transforms the input tags by the input transform.  If a fourth\n\
     argument is present, then the inverse of the transform is used.  The\n\
     transformed tags are written to output.tag if specified, otherwise\n\
     input.tag is overwritten.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_Status              status;
    VIO_STR              input_filename, output_filename, transform_filename;
    VIO_STR              dummy;
    int                 i;
    VIO_BOOL             invert;
    VIO_General_transform   transform;
    int                 n_volumes, n_tag_points;
    VIO_Real                **tags_volume1, **tags_volume2, *weights;
    int                 *structure_ids, *patient_ids;
    VIO_STR              *labels;

    status = VIO_OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &transform_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );

    invert = get_string_argument( "", &dummy );

    if( input_tag_file( input_filename, &n_volumes, &n_tag_points,
                        &tags_volume1, &tags_volume2, &weights,
                        &structure_ids, &patient_ids, &labels ) != VIO_OK )
        return( 1 );

    if( input_transform_file( transform_filename, &transform ) != VIO_OK )
        return( 1 );

    if( status == VIO_OK )
    {
        for_less( i, 0, n_tag_points )
        {
            if( invert )
            {
                general_inverse_transform_point( &transform,
                 tags_volume1[i][VIO_X], tags_volume1[i][VIO_Y], tags_volume1[i][VIO_Z],
                 &tags_volume1[i][VIO_X], &tags_volume1[i][VIO_Y], &tags_volume1[i][VIO_Z]);
            }
            else
            {
                general_transform_point( &transform,
                 tags_volume1[i][VIO_X], tags_volume1[i][VIO_Y], tags_volume1[i][VIO_Z],
                 &tags_volume1[i][VIO_X], &tags_volume1[i][VIO_Y], &tags_volume1[i][VIO_Z]);
            }

            if( n_volumes == 2 )
            {
                if( invert )
                {
                    general_inverse_transform_point( &transform,
                     tags_volume2[i][VIO_X], tags_volume2[i][VIO_Y], tags_volume2[i][VIO_Z],
                     &tags_volume2[i][VIO_X], &tags_volume2[i][VIO_Y],
                     &tags_volume2[i][VIO_Z] );
                }
                else
                {
                    general_transform_point( &transform,
                     tags_volume2[i][VIO_X], tags_volume2[i][VIO_Y], tags_volume2[i][VIO_Z],
                     &tags_volume2[i][VIO_X], &tags_volume2[i][VIO_Y],
                     &tags_volume2[i][VIO_Z] );
                }
            }
        }
    }

    status = output_tag_file( output_filename, (char *) NULL,
                              n_volumes, n_tag_points,
                              tags_volume1, tags_volume2, weights,
                              structure_ids, patient_ids, labels );

    if( status == VIO_OK )
        delete_general_transform( &transform );

    if( status == VIO_OK )
        free_tag_points( n_volumes, n_tag_points,
                         tags_volume1, tags_volume2, weights,
                         structure_ids, patient_ids, labels );

    return( status != VIO_OK );
}
