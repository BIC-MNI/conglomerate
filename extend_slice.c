#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.mnc  output.mnc  output.mnc x|y|z slice_number\n\
\n\
     Expands the specified slice to fill the volume.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename, output_filename, axis_name;
    int                  voxel_pos, a1, a2, axis;
    int                  x, y, z, sizes[MAX_DIMENSIONS];
    int                  v, voxel[MAX_DIMENSIONS];
    Volume               volume;
    Real                 voxel_value;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_string_argument( "", &axis_name ) ||
        !get_int_argument( 0, &voxel_pos ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    axis = axis_name[0] - 'x';
    if( axis < 0 || axis >= N_DIMENSIONS )
    {
        usage( argv[0] );
        return( 1 );
    }

    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    for_less( voxel[a1], 0, sizes[a1] )
    {
        for_less( voxel[a2], 0, sizes[a2] )
        {
            for_less( v, 0, sizes[axis] )
            {
                if( v == voxel_pos )
                    continue;

                voxel[axis] = voxel_pos;
                voxel_value = get_volume_voxel_value( volume, voxel[X],
                                 voxel[Y], voxel[Z], 0, 0 );
                voxel[axis] = v;
                set_volume_voxel_value( volume, voxel[X],
                                 voxel[Y], voxel[Z], 0, 0, voxel_value );
            }
        }
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, volume_filename,
                                   "Extended Slice\n", NULL );

    return( 0 );
}
