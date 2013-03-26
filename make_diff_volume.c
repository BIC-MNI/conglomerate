#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input1.mnc  min1 max1  input2.mnc  min2 max2 output.mnc\n\
\n\
\n\
     Creates the 4 class volume, depending on whether a voxel is within\n\
     (min1,max1) and (min2,max2).\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume1_filename, volume2_filename, output_filename;
    VIO_Real                 min1, max1, min2, max2, value1, value2;
    VIO_Real                 neither, first_only, second_only, both, value;
    BOOLEAN              in_one, in_two;
    int                  v0, v1, v2, v3, v4;
    VIO_Volume               volume1, volume2, output_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume1_filename ) ||
        !get_real_argument( 0.0, &min1 ) ||
        !get_real_argument( 0.0, &max1 ) ||
        !get_string_argument( NULL, &volume2_filename ) ||
        !get_real_argument( 0.0, &min2 ) ||
        !get_real_argument( 0.0, &max2 ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &neither );
    (void) get_real_argument( 50.0, &first_only );
    (void) get_real_argument( 100.0, &second_only );
    (void) get_real_argument( 200.0, &both );

    if( input_volume( volume1_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume1, NULL ) != OK )
        return( 1 );

    if( input_volume( volume2_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume2, NULL ) != OK )
        return( 1 );

    output_volume = copy_volume_definition( volume1, NC_BYTE, FALSE, 0.0, 0.0 );

    BEGIN_ALL_VOXELS( volume1, v0, v1, v2, v3, v4 )

        value1 = get_volume_real_value( volume1, v0, v1, v2, v3, v4 );
        value2 = get_volume_real_value( volume2, v0, v1, v2, v3, v4 );

        in_one = (min1 > max1 || min1 <= value1 && value1 <= max1);
        in_two = (min2 > max2 || min2 <= value2 && value2 <= max2);

        if( !in_one && !in_two )
            value = neither;
        else if( in_one && !in_two )
            value = first_only;
        else if( !in_one && in_two )
            value = second_only;
        else
            value = both;

        set_volume_real_value( output_volume, v0, v1, v2, v3, v4, value );

    END_ALL_VOXELS

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, output_volume, volume1_filename,
                                   "make_diff_volumes\n", NULL );

    return( 0 );
}
