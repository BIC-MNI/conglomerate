#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <images.h>

private  Volume  convert_pixels_to_volume(
    int            n_slices,
    pixels_struct  pixels[] );

int  main(
    int   argc,
    char  *argv[] )
{
    Volume         volume;
    char           *input_filename, *output_filename;
    pixels_struct  pixels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s input.rgb output.mnc\n", argv[0] );
        return( 1 );
    }

    if( input_rgb_file( input_filename, &pixels ) != OK )
        return( 1 );

    volume = convert_pixels_to_volume( 1, &pixels );

    if( volume != NULL )
    {
        (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                              0.0, 0.0,
                              volume, "Converted from pixels",
                              (minc_output_options *) NULL );

        delete_volume( volume );
    }

    return( 0 );
}

private  Volume  convert_pixels_to_volume(
    int            n_slices,
    pixels_struct  pixels[] )
{
    int      i, x, y, z, c, sizes[4];
    int      r, g, b;
    Volume   volume;
    Colour   colour;

    volume = create_volume( N_DIMENSIONS, XYZ_dimension_names,
                            NC_BYTE, FALSE, 0.0, 0.0 );

    set_volume_real_range( volume, 0.0, 1.0 );

    for_less( i, 1, n_slices )
    {
        if( pixels[i].x_size != pixels[0].x_size ||
            pixels[i].y_size != pixels[0].y_size )
        {
            print_error( "Pixel sizes do not match.\n" );
            return( NULL );
        }
    }

    sizes[X] = pixels[0].x_size;
    sizes[Y] = pixels[0].y_size;
    sizes[Z] = n_slices;

    set_volume_sizes( volume, sizes );

    alloc_volume_data( volume );

    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    for_less( z, 0, sizes[Z] )
    {
        colour = PIXEL_RGB_COLOUR(pixels[z],x,y);

        r = get_Colour_r(colour);
        g = get_Colour_g(colour);
        b = get_Colour_b(colour);

        set_volume_voxel_value( volume, x, y, z, 0, 0, (Real) r );
        set_volume_voxel_value( volume, x, y, z, 1, 0, (Real) g );
        set_volume_voxel_value( volume, x, y, z, 2, 0, (Real) b );
    }

    return( volume );
}
