#include  <volume_io.h>
#include  <bicpl.h>
#include  <bicpl/images.h>

static  VIO_Volume  convert_pixels_to_volume(
    int            n_components,
    int            n_slices,
    int            n_dimensions,
    nc_type        vol_type,
    pixels_struct  pixels[] );

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Volume         volume;
    VIO_STR         input_filename, output_filename;
    int            n_slices, n_components;
    pixels_struct  *pixels;
    nc_type        vol_type;
    VIO_BOOL        two_d_allowed;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &n_components ) )
    {
        print( "Usage: %s output.mnc  3|4|23|24 input1.rgb input2.rgb ...\n", argv[0] );
        return( 1 );
    }

    if( n_components > 100 )
    {
        vol_type = NC_FLOAT;
        n_components -= 100;
    }
    else
        vol_type = NC_BYTE;

    if( n_components > 20 )
    {
        two_d_allowed = TRUE;
        n_components -= 20;
    }
    else
    {
        two_d_allowed = FALSE;
    }

    n_slices = 0;
    pixels = NULL;

    while( get_string_argument( "", &input_filename ) )
    {
        SET_ARRAY_SIZE( pixels, n_slices, n_slices+1, DEFAULT_CHUNK_SIZE );

        if( input_rgb_file( input_filename, &pixels[n_slices] ) != VIO_OK )
            return( 1 );

        ++n_slices;
    }

    volume = convert_pixels_to_volume( n_components, n_slices,
                                       (two_d_allowed && n_slices == 1) ? 2 : 3,
                                       vol_type, pixels );

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

static  VIO_Volume  convert_pixels_to_volume(
    int            n_components,
    int            n_slices,
    int            n_dimensions,
    nc_type        vol_type,
    pixels_struct  pixels[] )
{
    int      i, x, y, z, sizes[4];
    VIO_Real     r, g, b, a;
    static   char  *dim_names2[] = { MIxspace, MIyspace, MIvector_dimension };
    static   char  *dim_names3[] = { MIzspace,
                                     MIxspace, MIyspace, MIvector_dimension };
    VIO_Volume   volume;
    VIO_Colour   colour;

    volume = create_volume( n_dimensions+(n_components>1?1:0),
                            n_dimensions == 2 ? dim_names2 : dim_names3,
                            vol_type, FALSE, 0.0, 0.0 );

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

    if( n_dimensions == 2 )
    {
        sizes[0] = pixels[0].x_size;
        sizes[1] = pixels[0].y_size;
        sizes[2] = n_components;
    }
    else
    {
        sizes[0] = n_slices;
        sizes[1] = pixels[0].x_size;
        sizes[2] = pixels[0].y_size;
        sizes[3] = n_components;
    }

    set_volume_sizes( volume, sizes );

    alloc_volume_data( volume );

    for_less( x, 0, pixels[0].x_size )
    for_less( y, 0, pixels[0].y_size )
    for_less( z, 0, n_slices )
    {
        colour = PIXEL_RGB_COLOUR(pixels[z],x,y);

        if( n_components == 1 )
            r = (VIO_Real) get_Colour_luminance( colour ) / 255.0;
        else
        {
            r = (VIO_Real) get_Colour_r(colour) / 255.0;
            g = (VIO_Real) get_Colour_g(colour) / 255.0;
            b = (VIO_Real) get_Colour_b(colour) / 255.0;
            a = (VIO_Real) get_Colour_a(colour) / 255.0;
        }
  
        if( n_dimensions == 2 )
        {
            set_volume_real_value( volume, x, y, 0, 0, 0, r );

            if( n_components > 1 )
            {
                set_volume_real_value( volume, x, y, 1, 0, 0, g );
                set_volume_real_value( volume, x, y, 2, 0, 0, b );
            }

            if( n_components == 4 )
                set_volume_real_value( volume, x, y, 3, 0, 0, a );
        }
        else
        {
            set_volume_real_value( volume, z, x, y, 0, 0, r );

            if( n_components > 1 )
            {
                set_volume_real_value( volume, z, x, y, 1, 0, g );
                set_volume_real_value( volume, z, x, y, 2, 0, b );
            }

            if( n_components == 4 )
                set_volume_real_value( volume, z, x, y, 3, 0, a );
        }
    }

    return( volume );
}
