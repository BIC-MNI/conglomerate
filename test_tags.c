#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *input_tags, *output_tags;
    char                 **labels;
    int                  i;
    int                  n_volumes, n_tag_points;
    Real                 **tags_volume1, **tags_volume2, *weights;
    int                  *structure_ids, *patient_ids;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_tags ) ||
        !get_string_argument( "", &output_tags ) )
    {
        print( "%s  input.tags output.tags\n", argv[0] );
        return( 1 );
    }

    if( input_tag_file( input_tags, &n_volumes, &n_tag_points,

                        &tags_volume1, &tags_volume2, &weights,
                        &structure_ids, &patient_ids, &labels ) != OK )
        return( 1 );

    (void) output_tag_file( output_tags, (char *) NULL,
                            n_volumes, n_tag_points,
                            tags_volume1, tags_volume2, weights,
                            structure_ids, patient_ids, labels );

    return( 0 );
}
