#include  <volume_io.h>
#include  <bicpl.h>
#include  <bicpl/images.h>

typedef enum { ADD, COMPOSITE } Composite_types;

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Volume             volume1, volume2;
    VIO_STR             input_filename1, input_filename2, output_filename;
    VIO_STR             method;
    int                x, y;
    int                sizes[VIO_MAX_DIMENSIONS];
    int                sizes2[VIO_MAX_DIMENSIONS];
    VIO_STR             dim_names4d[] = {
                                         MIzspace,
                                         MIxspace,
                                         MIyspace,
                                         MIvector_dimension };
    Real               rgba1[4], rgba2[4], weight;
    minc_input_options options;
    Composite_types    comp_type;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename1 ) ||
        !get_string_argument( NULL, &input_filename2 ) ||
        !get_string_argument( NULL, &method ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !equal_strings( method, "add" ) &&
        !equal_strings( method, "composite" ) )
    {
        print( "Usage: %s input1.mnc  input2.mnc  [add|composite] output.mnc\n", argv[0] );
        return( 1 );
    }

    if( equal_strings( method, "add" ) )
        comp_type = ADD;
    else
        comp_type = COMPOSITE;

    set_default_minc_input_options( &options );
    set_minc_input_vector_to_scalar_flag( &options, FALSE );
    set_minc_input_vector_to_colour_flag( &options, FALSE );

    if( input_volume( input_filename1, 4, dim_names4d,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume1, &options ) != OK )
        return( 1 );

    if( input_volume( input_filename2, 4, dim_names4d,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume2, &options ) != OK )
        return( 1 );

    get_volume_sizes( volume1, sizes );
    get_volume_sizes( volume2, sizes2 );

    if( sizes[0] != sizes2[0] ||
        sizes[1] != sizes2[1] ||
        sizes[2] != sizes2[2] ||
        sizes[3] != sizes2[3] )
    {
        int  dim;
        print_error( "Sizes do not match \n" );
        for_less( dim, 0, 4 )
            print( "%d %d\n", sizes[dim], sizes2[dim] );
        return( 1 );
    }

    rgba1[3] = 1.0;
    rgba2[3] = 1.0;

    for_less( x, 0, sizes[1] )
    {
        for_less( y, 0, sizes[2] )
        {
            get_volume_value_hyperslab( volume1, 0, x, y, 0, 0,
                                                 1, 1, 1, sizes[3], 0, rgba1 );
            get_volume_value_hyperslab( volume2, 0, x, y, 0, 0,
                                                 1, 1, 1, sizes[3], 0, rgba2 );

            if( comp_type == ADD )
            {
                rgba1[0] += rgba2[0];
                rgba1[1] += rgba2[1];
                rgba1[2] += rgba2[2];

                if( sizes[3] == 4 )
                    rgba1[3] += rgba2[3];

                if( rgba1[3] > 1.0 )
                    rgba1[3] = 1.0;
            }
            else
            {
                if( rgba2[3] >= 1.0 || rgba1[3] == 0.0 )
                {
                    rgba1[0] = rgba2[0];
                    rgba1[1] = rgba2[1];
                    rgba1[2] = rgba2[2];
                    if( sizes[3] == 4 )
                        rgba1[3] = rgba2[3];
                }
                else if( rgba2[3] == 0.0 )
                {
                    /* don't change rgba1 */
                }
                else
                {
                    weight = (1.0 - rgba2[3]) * rgba1[3];
                    rgba1[0] = rgba2[3] * rgba2[0] + weight * rgba1[0];
                    rgba1[1] = rgba2[3] * rgba2[1] + weight * rgba1[1];
                    rgba1[2] = rgba2[3] * rgba2[2] + weight * rgba1[2];

                    if( sizes[3] == 4 )
                    {
                        rgba1[3] = rgba2[3] + weight;

                        if( rgba1[3] > 0.0 )
                        {
                            rgba1[0] /= rgba1[3];
                            rgba1[1] /= rgba1[3];
                            rgba1[2] /= rgba1[3];
                        }
                    }
                }
            }

            set_volume_value_hyperslab( volume1, 0, x, y, 0, 0,
                                                 1, 1, 1, sizes[3], 0, rgba1 );
        }
    }

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, volume1, "Composited\n", NULL );

    return( 0 );
}
