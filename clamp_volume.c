#include  <internal_volume_io.h>
#include  <bicpl.h>

private   void  check_neighbours(
    Volume        volume,
    Smallest_int  ***flags,
    int           x,
    int           y,
    int           z,
    int           dx_min,
    int           dx_max,
    int           dy_min,
    int           dy_max,
    int           dz_min,
    int           dz_max,
    Real          min_voxel,
    Real          max_voxel,
    int           *n_lower,
    int           *n_higher );

typedef  enum  { BELOW_THRESHOLD, WITHIN_THRESHOLD, ABOVE_THRESHOLD }
               Threshold_types;

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    Volume               volume;
    Real                 min_threshold, max_threshold;
    Real                 min_voxel, max_voxel;
    Real                 set_low, set_high;
    int                  x, y, z, sizes[N_DIMENSIONS];
    int                  dx_min, dx_max, dy_min, dy_max, dz_min, dz_max;
    int                  n_lower, n_higher;
    int                  start, end, slice;
    Smallest_int         **flags[3], **tmp;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &min_threshold ) ||
        !get_real_argument( 0.0, &max_threshold ) )
    {
        print( "%s  input.mnc  output.mnc  min max\n", argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    get_volume_voxel_range( volume, &min_voxel, &max_voxel );

    set_low = convert_value_to_voxel( volume, min_threshold );
    set_low = ROUND( set_low );
    while( convert_voxel_to_value( volume, set_low ) > min_threshold )
        set_low -= 1.0;
    if( set_low < min_voxel )
        set_low = min_voxel;

    set_high = convert_value_to_voxel( volume, max_threshold );
    set_high = ROUND( set_high );
    while( convert_voxel_to_value( volume, set_high ) < max_threshold )
        set_high += 1.0;
    if( set_high > max_voxel )
        set_high = max_voxel;

    n_lower = 0;
    n_higher = 0;

    ALLOC2D( flags[0], sizes[1], sizes[2] );
    ALLOC2D( flags[1], sizes[1], sizes[2] );
    ALLOC2D( flags[2], sizes[1], sizes[2] );

    initialize_progress_report( &progress, FALSE, sizes[0] * sizes[1],
                                "Clamping" );

    for_less( x, 0, sizes[0] )
    {
        tmp = flags[0];
        flags[0] = flags[1];
        flags[1] = flags[2];
        flags[2] = tmp;

        if( x == 0 )
            start = 0;
        else
            start = 1;
        if( x == sizes[0]-1 )
            end = 0;
        else
            end = 1;

        for_inclusive( slice, start, end )
        {
            for_less( y, 0, sizes[1] )
            for_less( z, 0, sizes[2] )
            {
                Threshold_types  type;
                Real             value;

                GET_VALUE_3D( value, volume, x + slice, y, z );

                if( value < min_threshold )
                    type = BELOW_THRESHOLD;
                else if( value > max_threshold )
                    type = ABOVE_THRESHOLD;
                else
                    type = WITHIN_THRESHOLD;
                flags[1+slice][y][z] = (Smallest_int) type;
            }
        }

        if( x == 0 )
            dx_min = 0;
        else
            dx_min = -1;
        if( x == sizes[0]-1 )
            dx_max = 0;
        else
            dx_max = 1;
        for_less( y, 0, sizes[1] )
        {
            if( y == 0 )
                dy_min = 0;
            else
                dy_min = -1;
            if( y == sizes[1]-1 )
                dy_max = 0;
            else
                dy_max = 1;
            for_less( z, 0, sizes[2] )
            {
                if( z == 0 )
                    dz_min = 0;
                else
                    dz_min = -1;
                if( z == sizes[2]-1 )
                    dz_max = 0;
                else
                    dz_max = 1;

                check_neighbours( volume, flags, x, y, z, dx_min, dx_max,
                                  dy_min, dy_max, dz_min, dz_max,
                                  set_low, set_high, &n_lower, &n_higher );
            }

            update_progress_report( &progress, x * sizes[1] + y + 1 );
        }
    }

    FREE2D( flags[0] );
    FREE2D( flags[1] );
    FREE2D( flags[2] );

    terminate_progress_report( &progress );

    print( "N lower changed: %d\n", n_lower );
    print( "N higher changed: %d\n", n_higher );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, volume, input_filename,
                             "Clamped", (minc_output_options *) NULL );

    delete_volume( volume );

    return( 0 );
}

private   void  check_neighbours(
    Volume        volume,
    Smallest_int  ***flags,
    int           x,
    int           y,
    int           z,
    int           dx_min,
    int           dx_max,
    int           dy_min,
    int           dy_max,
    int           dz_min,
    int           dz_max,
    Real          min_voxel,
    Real          max_voxel,
    int           *n_lower,
    int           *n_higher )
{
    int              dx, dy, dz;
    BOOLEAN          change;
    Threshold_types  type, desired_type;

    desired_type = flags[1][y][z];
    if( desired_type == WITHIN_THRESHOLD )
        return;

    change = TRUE;
    for_inclusive( dx, dx_min, dx_max )
    {
        for_inclusive( dy, dy_min, dy_max )
        {
            for_inclusive( dz, dz_min, dz_max )
            {
                type = flags[1+dx][y+dy][z+dz];
                if( desired_type != type )
                {
                    change = FALSE;
                    break;
                }
            }
            if( !change ) break;
        }
        if( !change ) break;
    }

    if( change && desired_type == BELOW_THRESHOLD )
    {
        SET_VOXEL_3D( volume, x, y, z, min_voxel )
        ++(*n_lower);
    }
    else if( change && desired_type == ABOVE_THRESHOLD )
    {
        SET_VOXEL_3D( volume, x, y, z, max_voxel )
        ++(*n_higher);
    }
}
