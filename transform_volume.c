
#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.mnc  input.xfm  output.mnc\n\
\n\
     Changes the voxel_to_world transform by appending the input transform to\n\
     the existing one, writing the resulting volume in output.mnc.\n\
     This is an efficient method of transforming a volume to another space,\n\
     since there is no data loss;  the volume values stay the same.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING     input_filename, output_filename, transform_filename;
    Volume     volume;
    General_transform   transform, new_transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &transform_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }
 
    if( input_volume( input_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_transform_file( transform_filename, &transform ) != OK )
        return( 0 );

    concat_general_transforms( get_voxel_to_world_transform(volume),
                               &transform, &new_transform );

    set_voxel_to_world_transform( volume, &new_transform );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, volume, "Transformed", NULL );
    
    return( 0 );
}
