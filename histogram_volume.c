#include  <module.h>

#define  DEFAULT_N_INTERVALS  100

int  main(
    int   argc,
    char  *argv[] )
{
    int                  x, y, z, sizes[N_DIMENSIONS], x_size, y_size;
    Real                 value;
    Real                 min_value, max_value;
    char                 *input_volume_filename, *output_filename;
    lines_struct         *lines;
    object_struct        *object;
    histogram_struct     histogram;
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  volume_file output_file\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( DEFAULT_N_INTERVALS, &x_size );
    (void) get_int_argument( x_size, &y_size );

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );
    get_volume_real_range( volume, &min_value, &max_value );

    initialize_histogram( &histogram, (max_value - min_value) / 1000.0, 0.0 );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VALUE_3D( value, volume, x, y, z );
                add_to_histogram( &histogram, value );
            }
        }
    }

    object = create_object( LINES );

    lines = get_lines_ptr( object );

    create_histogram_line( &histogram, x_size, y_size, lines );

    (void) output_graphics_file( output_filename, ASCII_FORMAT, 1, &object );

    return( 0 );
}
