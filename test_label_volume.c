#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename;
    Volume               volume, mask_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) )
    {
        return( 1 );
    }

    if( input_volume_header_only( input_filename, 3, XYZ_dimension_names,
                                  &volume, NULL ) != OK )
        return( 1 );

    mask_volume = create_label_volume( volume, NC_UNSPECIFIED );

    if( load_label_volume( input_filename, mask_volume ) != OK )
        return( 1 );

    return( 0 );
}
