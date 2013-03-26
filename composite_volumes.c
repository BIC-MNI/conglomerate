#include  <volume_io.h>
#include  <bicpl.h>
#include  <bicpl/images.h>

#define  GRAY_STRING       "gray"
#define  HOT_STRING        "hot"
#define  SPECTRAL_STRING   "spectral"
#define  RED_STRING        "red"
#define  GREEN_STRING      "green"
#define  BLUE_STRING       "blue"
#define  USER_STRING       "user"

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Volume             volume, rgb_volume;
    VIO_STR             input_filename, output_filename;
    VIO_STR             under_colour_name, over_colour_name;
    int                n_dims, sizes[VIO_MAX_DIMENSIONS], dim, n_volumes;
    int                v[VIO_MAX_DIMENSIONS];
    Real               vr[VIO_MAX_DIMENSIONS];
    Real               vv[VIO_MAX_DIMENSIONS], xw, yw, zw;
    int                ivv[VIO_MAX_DIMENSIONS], x, y, z;
    VIO_STR             *dim_names, *dim_names_rgb;
    VIO_STR             coding_type_string;
    Real               value, low, high, r, g, b, a;
    Real               separations[VIO_MAX_DIMENSIONS], opacity;
    VIO_Colour             colour;
    VIO_Colour             over_colour, under_colour, new_col, old_col;
    VIO_General_transform  transform;
    Colour_coding_types  coding_type;
    colour_coding_struct colour_coding;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) )
    {
        print( "Usage: %s output.mnc\n", argv[0] );
        return( 1 );
    }

    n_volumes = 0;
    while( get_string_argument( NULL, &input_filename ) &&
           get_string_argument( NULL, &coding_type_string ) &&
           get_real_argument( 0.0, &low ) &&
           get_real_argument( 0.0, &high ) &&
           get_string_argument( NULL, &under_colour_name ) &&
           get_string_argument( NULL, &over_colour_name ) &&
           get_real_argument( 1.0, &opacity ) )
    {
        if( equal_strings( coding_type_string, GRAY_STRING ) )
            coding_type = GRAY_SCALE;
        else if( equal_strings( coding_type_string, HOT_STRING ) )
            coding_type = HOT_METAL;
        else if( equal_strings( coding_type_string, SPECTRAL_STRING ) )
            coding_type = SPECTRAL;
        else if( equal_strings( coding_type_string, RED_STRING ) )
            coding_type = RED_COLOUR_MAP;
        else if( equal_strings( coding_type_string, GREEN_STRING ) )
            coding_type = GREEN_COLOUR_MAP;
        else if( equal_strings( coding_type_string, BLUE_STRING ) )
            coding_type = BLUE_COLOUR_MAP;

        under_colour = convert_string_to_colour( under_colour_name );
        over_colour = convert_string_to_colour( over_colour_name );

        initialize_colour_coding( &colour_coding, coding_type,
                                  under_colour, over_colour, low, high );

        if( input_volume( input_filename, -1, File_order_dimension_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          TRUE, &volume, NULL ) != OK )
            return( 1 );

        if( n_volumes == 0 )
        {
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

                v[n_dims] = 0;
                set_volume_real_value( rgb_volume,v[0],v[1],v[2],v[3],v[4],0.0);
                v[n_dims] = 1;
                set_volume_real_value( rgb_volume,v[0],v[1],v[2],v[3],v[4],0.0);
                v[n_dims] = 2;
                set_volume_real_value( rgb_volume,v[0],v[1],v[2],v[3],v[4],0.0);
                v[n_dims] = 3;
                set_volume_real_value( rgb_volume,v[0],v[1],v[2],v[3],v[4],0.0);
                v[n_dims] = 0;

            END_ALL_VOXELS
        }

        for_less( x, 0, sizes[X] )
        {
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            v[0] = x;
            v[1] = y;
            v[2] = z;
            vr[0] = (Real) x;
            vr[1] = (Real) y;
            vr[2] = (Real) z;
            convert_voxel_to_world( rgb_volume, vr, &xw, &yw, &zw );
            convert_world_to_voxel( volume, xw, yw, zw, vv );

            convert_real_to_int_voxel( 3, vv, ivv );
            if( !int_voxel_is_within_volume( volume, ivv ) )
                continue;

            value = get_volume_real_value( volume, ivv[0], ivv[1], ivv[2], ivv[3],
                                           ivv[4]);

            new_col = get_colour_code( &colour_coding, value );

            a = get_Colour_a_0_1(new_col);

            if( a * opacity < 1.0 )
            {
                r = get_Colour_r_0_1(new_col);
                g = get_Colour_g_0_1(new_col);
                b = get_Colour_b_0_1(new_col);
                a = get_Colour_a_0_1(new_col);
                new_col = make_rgba_Colour_0_1( r, g, b, a * opacity );

                v[n_dims] = 0;
                r = get_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4] );

                v[n_dims] = 1;
                g = get_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4]);

                v[n_dims] = 2;
                b = get_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4]);

                v[n_dims] = 3;
                a = get_volume_real_value( rgb_volume, v[0], v[1], v[2], v[3], v[4]);

                old_col = make_rgba_Colour_0_1( r, g, b, a );

                COMPOSITE_COLOURS( colour, new_col, old_col );
            }
            else
                colour = new_col;

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

            v[n_dims] = 0;
        }
        print( "%d/%d\n", x+1, sizes[0] );
        }

        delete_volume( volume );

        ++n_volumes;
    }

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0,
                          rgb_volume, "Composited\n", NULL );

    return( 0 );
}
