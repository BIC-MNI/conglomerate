#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    FILE               *file;
    Volume             volume, rgb_volume;
    STRING             input_filename, colour_map_filename, output_filename;
    int                v[MAX_DIMENSIONS], i, n_labels, int_value, index;
    int                int_min_value, int_max_value;
    int                n_dims, sizes[MAX_DIMENSIONS], dim;
    STRING             *dim_names, *dim_names_rgb;
    Real               value, min_value, max_value, red, green, blue, alpha;
    Real               separations[MAX_DIMENSIONS];
    Colour             colour, *label_map;
    General_transform  transform;

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
        label_map[i] = make_rgba_Colour( 0, 0, 0, 0 );

    if( open_file( colour_map_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    while( input_int( file, &index ) == OK )
    {
        if( input_real( file, &red ) != OK ||
            input_real( file, &green ) != OK ||
            input_real( file, &blue ) != OK ||
            input_real( file, &alpha ) != OK )
        {
            print_error( "Error loading labels colour map.\n" );
            return( 1 );
        }

        if( index >= int_min_value && index < int_max_value )
        {
            label_map[index-int_min_value] = make_rgba_Colour_0_1(
                                               red, green, blue, alpha);
        }
    }

    (void) close_file( file );

    n_dims = get_volume_n_dimensions( volume );
    get_volume_separations( volume, separations );
    get_volume_sizes( volume, sizes );

    dim_names = get_volume_dimension_names( volume );

    ALLOC( dim_names_rgb, n_dims+1 );

    for_less( dim, 0, n_dims )
        dim_names_rgb[dim] = dim_names[dim];
    dim_names_rgb[n_dims] = MIvector_dimension;

    rgb_volume =  create_volume( n_dims+1, dim_names_rgb,
                                 NC_BYTE, FALSE, 0.0, 0.0 );
    sizes[n_dims] = 4;
    set_volume_sizes( rgb_volume, sizes );
    alloc_volume_data( rgb_volume );

    separations[n_dims] = 1.0;
    set_volume_separations( rgb_volume, separations );

    set_volume_real_range( rgb_volume, 0.0, 1.0 );

    copy_general_transform( get_voxel_to_world_transform(volume),
                            &transform );

    set_voxel_to_world_transform( rgb_volume, &transform );

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        value = get_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4] );

        int_value = ROUND( value );

        if( int_min_value <= int_value && int_value <= int_max_value )
            colour = label_map[int_value-int_min_value];
        else
            colour = BLACK;

        v[n_dims] = 0;
        set_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4],
                               get_Colour_r_0_1(colour) );

        v[n_dims] = 1;
        set_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4],
                               get_Colour_g_0_1(colour) );

        v[n_dims] = 2;
        set_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4],
                               get_Colour_b_0_1(colour) );

        v[n_dims] = 3;
        set_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4],
                               get_Colour_a_0_1(colour) );

    END_ALL_VOXELS

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0,
                          rgb_volume, "Converted from labels\n",
                          (minc_output_options *) NULL );

    return( 0 );
}
