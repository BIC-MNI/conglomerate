#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    FILE           *file;
    Volume         volume, rgb_volume;
    STRING         input_filename, colour_map_filename, output_filename;
    int            v0, v1, v2, v3, v4, i, n_labels, int_value, index;
    int            int_min_value, int_max_value;
    Real           value, min_value, max_value, red, green, blue;
    Colour         colour, *label_map;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &colour_map_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s input.mnc  colour_map.txt  output.mnc\n", argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    min_value = get_volume_real_min( volume );
    max_value = get_volume_real_max( volume );

    int_min_value = FLOOR( min_value );
    int_max_value = CEILING( max_value );

    n_labels = int_max_value - int_min_value + 1;

    ALLOC( label_map, n_labels );

    for_less( i, 0, n_labels )
        label_map[i] = BLACK;

    if( open_file( colour_map_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    while( input_int( file, &index ) == OK )
    {
        if( input_real( file, &red ) != OK ||
            input_real( file, &green ) != OK ||
            input_real( file, &blue ) != OK )
        {
            print_error( "Error loading labels colour map.\n" );
            return( 1 );
        }

        if( index >= int_min_value && index < int_max_value )
        {
            label_map[index-int_min_value] = make_Colour_0_1( red, green, blue);
        }
    }

    (void) close_file( file );

    rgb_volume = copy_volume_definition( volume, NC_LONG, FALSE, 0.0, 0.0 );

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )

        value = get_volume_real_value( volume, v0, v1, v2, v3, v4 );

        int_value = ROUND( value );

        if( int_min_value <= int_value && int_value <= int_max_value )
            colour = label_map[int_value-int_min_value];
        else
            colour = BLACK;

        set_volume_real_value( rgb_volume, v0, v1, v2, v3, v4, colour );

    END_ALL_VOXELS

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0,
                          rgb_volume, "Converted from labels\n",
                          (minc_output_options *) NULL );

    return( 0 );
}
