#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    Volume     volume;
    STRING     input_filename, output_filename;
    char       history[EXTREMELY_LARGE_STRING_SIZE];
    int        x_size, y_size, z_size, sizes[MAX_DIMENSIONS];
    int        n_voxels, x, y, z, i, j, k;
    int        end_x, end_y, end_z;
    Real       value, voxel_value, avg;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_int_argument( 0, &x_size ) ||
        !get_int_argument( 0, &y_size ) ||
        !get_int_argument( 0, &z_size ) )
    {
        print( "Usage: %s input.mnc output.mnc  nx_voxels_per_block ny_voxels_per_block nz_voxels_per_block\n", argv[0] );
        return( 1 );
    }

    /*--- input the volume */

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    /*--- compute the number of voxels in each average */

    n_voxels = x_size * y_size * z_size;

    /*--- step through volume in steps of averaging size */

    for( x = 0;  x < sizes[X];  x += x_size )
    {
        end_x = MIN( sizes[X], x + x_size );
        for( y = 0;  y < sizes[Y];  y += y_size )
        {
            end_y = MIN( sizes[Y], y + y_size );
            for( z = 0;  z < sizes[Z];  z += z_size )
            {
                end_z = MIN( sizes[Z], z + z_size );

                /*--- get the average of the block of voxels */

                avg = 0.0;
                for_less( i, x, end_x )
                for_less( j, y, end_y )
                for_less( k, z, end_z )
                {
                    value = get_volume_real_value( volume, i, j, k, 0, 0 );
                    avg += value;
                }

                avg /= (Real) n_voxels;
                voxel_value = convert_value_to_voxel( volume, avg );

                /*--- store the average in the block of voxels */

                for_less( i, x, end_x )
                for_less( j, y, end_y )
                for_less( k, z, end_z )
                {
                    set_volume_real_value( volume, i, j, k, 0, 0, voxel_value );
                }
            }
        }
    }

    (void) sprintf( history, "%s %s %s %d %d %d\n",
                    argv[0], input_filename, output_filename,
                    x_size, y_size, z_size );

    if( output_modified_volume( output_filename, NC_UNSPECIFIED,
             FALSE, 0.0, 0.0, volume, input_filename,
             history, (minc_output_options *) NULL ) != OK )
        return( 1 );

    return( 0 );
}
