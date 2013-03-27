#include  <volume_io.h>
#include  <bicpl.h>

#define   OFFSET  0.5625

private  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s input.mnc output.mnc  \n\
\n\
     Flips the brain about the anatomical origin\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR     input_filename, output_filename;
    VIO_Volume     volume, out_volume;
    int        v0, v1, v2, v3, v4;
    VIO_Real       value, xw, yw, zw;
    VIO_Real       v[MAX_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != VIO_OK )
        return( 1 );

    out_volume = copy_volume_definition( volume, NC_UNSPECIFIED, FALSE,
                                         0.0, 0.0 );

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )

        v[0] = (VIO_Real) v0;
        v[1] = (VIO_Real) v1;
        v[2] = (VIO_Real) v2;

        convert_voxel_to_world( volume, v, &xw, &yw, &zw );

        xw = OFFSET + -(xw - OFFSET);

        evaluate_volume_in_world( volume, xw, yw, zw, 0, FALSE, 0.0,
                                  &value, NULL, NULL, NULL,
                                          NULL, NULL, NULL,
                                          NULL, NULL, NULL );

        set_volume_real_value( out_volume, v0, v1, v2, v3, v4, value);

    END_ALL_VOXELS

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          out_volume, NULL, NULL );

    return( 0 );
}
