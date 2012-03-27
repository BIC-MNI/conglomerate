#include  <volume_io.h>
#include  <bicpl.h>
#include  <bicpl/images.h>

private  Status  load_colour_map(
    STRING           filename,
    int              *n_colours,
    Colour           *colours[] );

int  main(
    int   argc,
    char  *argv[] )
{
    Volume             volume, rgb_volume;
    STRING             input_filename, colour_map_filename, output_filename;
    int                v[MAX_DIMENSIONS], i, int_value;
    int                n_dims, sizes[MAX_DIMENSIONS], dim;
    STRING             *dim_names, *dim_names_rgb;
    Real               value;
    Real               separations[MAX_DIMENSIONS];
    Colour             colour, *label_map;
    General_transform  transform;
    int                n_colour_comps, n_colours;

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

    if( load_colour_map( colour_map_filename, &n_colours, &label_map ) != OK )
        return( 1 );

    n_colour_comps = 3;
    for_less( i, 0, n_colours )
    {
        if( get_Colour_a_0_1(label_map[i]) != 1.0 )
            n_colour_comps = 4;
    }

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
    sizes[n_dims] = n_colour_comps;
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

        if( 0 <= int_value && int_value < n_colours )
            colour = label_map[int_value];
        else
            colour = label_map[0];

        v[n_dims] = 0;
        set_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4],
                               get_Colour_r_0_1(colour) );

        v[n_dims] = 1;
        set_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4],
                               get_Colour_g_0_1(colour) );

        v[n_dims] = 2;
        set_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4],
                               get_Colour_b_0_1(colour) );

        if( n_colour_comps == 4 )
        {
            v[n_dims] = 3;
            set_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4],
                                   get_Colour_a_0_1(colour) );
        }

    END_ALL_VOXELS

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0,
                          rgb_volume, "Converted from labels\n",
                          (minc_output_options *) NULL );

    return( 0 );
}

private  Status  load_colour_map(
    STRING           filename,
    int              *n_colours,
    Colour           *colours[] )
{
    Status   status;
    FILE     *file;
    Colour   col;
    STRING   line;
    int      index, i;

    if( open_file_with_default_suffix( filename, "map",
                                       READ_FILE, ASCII_FORMAT, &file ) != OK )
        return( ERROR );

    *n_colours = 0;
    *colours = NULL;

    status = OK;

    while( input_int( file, &index ) == OK )
    {
        if( input_line( file, &line ) != OK )
        {
            print_error( "Error loading labels colour map.\n" );
            status = ERROR;
            break;
        }

        col = convert_string_to_colour( line );

        delete_string( line );

        if( index >= *n_colours )
        {
            SET_ARRAY_SIZE( *colours, *n_colours, index+1, DEFAULT_CHUNK_SIZE );
            for_less( i, *n_colours, index )
                (*colours)[i] = 0;
            *n_colours = index+1;
        }

        (*colours)[index] = col;
    }

    (void) close_file( file );

    return( status );
}
