#include  <volume_io.h>
#include  <special_geometry.h>

static  void  usage(
    VIO_STR  executable )
{
    static  VIO_STR  usage_str = "\n\
Usage: %s  input.tag  output.tag  ratio ...\n\
\n\
     Interpolates the second set of tags towards the first set.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               input_tags_filename, output_tags_filename;
    int                  tag, dim;
    VIO_Real                 ratio;
    int                  n_volumes, n_tag_points, *structure_ids, *patient_ids;
    VIO_Real                 **tags1, **tags2, *weights;
    VIO_STR               *labels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_tags_filename ) ||
        !get_string_argument( NULL, &output_tags_filename ) ||
        !get_real_argument( 0.0, &ratio ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_tag_file( input_tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, &weights, &structure_ids,
                        &patient_ids, &labels ) != OK )
        return( 1 );

    for_less( tag, 0, n_tag_points )
    {
        for_less( dim, 0, VIO_N_DIMENSIONS )
           tags2[tag][dim] = INTERPOLATE( ratio, tags2[tag][dim],
                                                 tags1[tag][dim] );
    }

    if( output_tag_file( output_tags_filename, "Interpolated",
                         n_volumes, n_tag_points, tags1, tags2,
                         weights, structure_ids,
                         patient_ids, labels ) != OK )
        return( 1 );

    free_tag_points( n_volumes, n_tag_points, tags1, tags2,
                     weights, structure_ids, patient_ids, labels );

    return( 0 );
}
