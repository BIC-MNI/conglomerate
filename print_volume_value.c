
#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING     input_filename;
    int        x, y, z, sizes[N_DIMENSIONS], n_done, i;
    int        min_v, max_v, *counts;
    Real       value, min_voxel, max_voxel, voxel;
    Volume     volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        return( 1 );
    }
 
    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    n_done = 0;

    while( get_int_argument( 0, &x ) &&
           get_int_argument( 0, &y ) &&
           get_int_argument( 0, &z ) )
    {
        if( x >= 0 && x < sizes[X] && y >= 0 && y < sizes[Y] &&
            z >= 0 && z < sizes[Z] )
        {
            value = get_volume_real_value( volume, x, y, z, 0, 0 );
            print( "%g\n", value );
        }
        else
            print( "Outside.\n" );
        ++n_done;
    }

    if( n_done == 0 )
    {
        get_volume_voxel_range( volume, &min_voxel, &max_voxel );

        min_v = FLOOR( min_voxel );
        max_v = CEILING( max_voxel );

        ALLOC( counts, max_v - min_v + 1 );

        for_inclusive( i, min_v, max_v )
            counts[i-min_v] = 0;

        for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            voxel = get_volume_voxel_value( volume, x, y, z, 0, 0 );
            ++counts[ROUND(voxel) - min_v];
        }

        for_inclusive( i, min_v, max_v )
        {
            if( counts[i-min_v] > 0 )
            {
                print( "%g : %d\n", convert_voxel_to_value(volume,(Real)i),
                       counts[i-min_v] );
            }
        }
    }


    return( 0 );
}
