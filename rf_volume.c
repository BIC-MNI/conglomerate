#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename;
    STRING               output_filename;
    Real                 x, y, z, xc, yc, zc, xp, yp, zp, len, pos;
    Real                 value, scaling, min_value, max_value;
    int                  v[MAX_DIMENSIONS], sizes[MAX_DIMENSIONS];
    Real                 voxel[MAX_DIMENSIONS], scale;
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &x ) ||
        !get_real_argument( 0.0, &y ) ||
        !get_real_argument( 0.0, &z ) ||
        !get_real_argument( 0.0, &scaling ) )
    {
        return( 1 );
    }

    len = sqrt( x * x + y * y + z * z );
    x /= len;
    y /= len;
    z /= len;

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );
    get_volume_real_range( volume, &min_value, &max_value );

    voxel[0] = (Real) (sizes[0]-1) / 2.0;
    voxel[1] = (Real) (sizes[1]-1) / 2.0;
    voxel[2] = (Real) (sizes[2]-1) / 2.0;

    convert_voxel_to_world( volume, voxel, &xc, &yc, &zc );
    
    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        voxel[0] = (Real) v[0];
        voxel[1] = (Real) v[1];
        voxel[2] = (Real) v[2];

        convert_voxel_to_world( volume, voxel, &xp, &yp, &zp );

        pos = (xp - xc) * x + (yp - yc) * y + (zp - zc) * z;

        scale = 1.0 + pos * scaling;
        value = get_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4] );
        value *= scale;
        if( value < min_value )
            value = min_value;
        else if( value > max_value )
            value = max_value;
        set_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4], value );

    END_ALL_VOXELS

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, volume, volume_filename,
                          "Scaled\n", NULL );

    return( 0 );
}
