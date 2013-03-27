#include  <mni.h>

#define  GRAY_STRING       "gray"
#define  HOT_STRING        "hot"
#define  SPECTRAL_STRING   "spectral"

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_volume_filename, *object_filename;
    char                 *coding_type_string;
    VIO_Real                 low, high;
    VIO_BOOL              low_present, high_present;
    File_formats         format;
    VIO_Volume               volume;
    int                  i, n_objects;
    object_struct        **objects;
    Colour_coding_types  coding_type;
    colour_coding_struct colour_coding;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &object_filename ) )
    {
        print( "Usage: %s  volume_file  object_file  [gray|hot|spectral]\n",
               argv[0] );
        print( "       [low high]\n" );
        return( 1 );
    }

    (void) get_string_argument( "gray", &coding_type_string );
    low_present = get_real_argument( 0.0, &low );
    high_present = get_real_argument( 0.0, &high );

    if( strcmp( coding_type_string, GRAY_STRING ) == 0 )
        coding_type = GRAY_SCALE;
    else if( strcmp( coding_type_string, HOT_STRING ) == 0 )
        coding_type = HOT_METAL;
    else if( strcmp( coding_type_string, SPECTRAL_STRING ) == 0 )
        coding_type = SPECTRAL;
    else
    {
        print( "Invalid coding type: %s\n", coding_type_string );
        return( 1 );
    }

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    if( input_graphics_file( object_filename,
                             &format, &n_objects, &objects ) != VIO_OK )
        return( 1 );

    if( !low_present )
        low = get_volume_real_min( volume );

    if( !high_present )
        high = get_volume_real_max( volume );

    initialize_colour_coding( &colour_coding, coding_type, BLACK, WHITE,
                              low, high );

    for_less( i, 0, n_objects )
        colour_code_object( volume, 0, &colour_coding, objects[i] );

    if( output_graphics_file( object_filename, format, n_objects, objects )
                                != VIO_OK )
        return( 1 );

    delete_object_list( n_objects, objects );

    return( 0 );
}
