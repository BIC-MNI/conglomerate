#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
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
    Status              status;
    STRING              input_filename, output_filename, transform_filename;
    STRING              dummy;
    int                 i;
    BOOLEAN             invert;
    General_transform   transform;
    int                 n_volumes, n_tag_points;
    Real                **tags_volume1, **tags_volume2, *weights;
    int                 *structure_ids, *patient_ids;
    STRING              *labels;

    status = OK;

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
                        &structure_ids, &patient_ids, &labels ) != OK )
        return( 1 );

    if( input_transform_file( transform_filename, &transform ) != OK )
        return( 1 );

    if( status == OK )
    {
        for_less( i, 0, n_tag_points )
        {
            if( invert )
            {
                general_inverse_transform_point( &transform,
                 tags_volume1[i][X], tags_volume1[i][Y], tags_volume1[i][Z],
                 &tags_volume1[i][X], &tags_volume1[i][Y], &tags_volume1[i][Z]);
            }
            else
            {
                general_transform_point( &transform,
                 tags_volume1[i][X], tags_volume1[i][Y], tags_volume1[i][Z],
                 &tags_volume1[i][X], &tags_volume1[i][Y], &tags_volume1[i][Z]);
            }

            if( n_volumes == 2 )
            {
                if( invert )
                {
                    general_inverse_transform_point( &transform,
                     tags_volume2[i][X], tags_volume2[i][Y], tags_volume2[i][Z],
                     &tags_volume2[i][X], &tags_volume2[i][Y],
                     &tags_volume2[i][Z] );
                }
                else
                {
                    general_transform_point( &transform,
                     tags_volume2[i][X], tags_volume2[i][Y], tags_volume2[i][Z],
                     &tags_volume2[i][X], &tags_volume2[i][Y],
                     &tags_volume2[i][Z] );
                }
            }
        }
    }

    status = output_tag_file( output_filename, (char *) NULL,
                              n_volumes, n_tag_points,
                              tags_volume1, tags_volume2, weights,
                              structure_ids, patient_ids, labels );

    if( status == OK )
        delete_general_transform( &transform );

    if( status == OK )
        free_tag_points( n_volumes, n_tag_points,
                         tags_volume1, tags_volume2, weights,
                         structure_ids, patient_ids, labels );

    return( status != OK );
}
