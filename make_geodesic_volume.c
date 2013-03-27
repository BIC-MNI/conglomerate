
#include  <volume_io.h>
#include  <bicpl.h>

static  void  label_surface_voxels(
    VIO_Volume    volume,
    VIO_Volume    geodesic,
    VIO_Real      threshold,
    VIO_Real      surface_value,
    VIO_Real      nonsurface_value,
    VIO_Real      tolerance );

static  void  find_nearest_voxel(
    VIO_Volume   volume,
    VIO_Real     voxel[],
    VIO_Real     value,
    int      nearest[] );

static  VIO_BOOL  volume_range_contains_threshold(
    VIO_Volume   volume,
    VIO_Real     min_voxel[],
    VIO_Real     max_voxel[],
    VIO_Real     threshold );

static  void  compute_geodesic_volume(
    VIO_Volume   volume,
    int      origin[],
    VIO_Real     surface_value,
    VIO_Real     nonsurface_value );

static  void  usage(
    char   executable[] )
{
    VIO_STR   usage_str = "\n\
Usage: %s   input.mnc  output.mnc   threshold  x y z \n\
            [x_size]  [y_size]  [z_size]\n\
\n\
     Creates the geodesic volume with respect to the (x, y, z) points.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR              input_filename, output_filename, *dim_names;
    VIO_Volume              volume, geodesic;
    int                 dim, origin[VIO_N_DIMENSIONS];
    int                 sizes[VIO_N_DIMENSIONS], geo_sizes[VIO_N_DIMENSIONS];
    VIO_Real                threshold, x_world, y_world, z_world;
    VIO_Real                voxel[VIO_N_DIMENSIONS], max_voxel, tolerance;
    VIO_Transform           new_to_old;
    VIO_General_transform   *volume_transform, mod_transform, new_transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &threshold ) ||
        !get_real_argument( 0.0, &x_world ) ||
        !get_real_argument( 0.0, &y_world ) ||
        !get_real_argument( 0.0, &z_world ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -1, &geo_sizes[VIO_X] );
    (void) get_int_argument( -1, &geo_sizes[VIO_Y] );
    (void) get_int_argument( -1, &geo_sizes[VIO_Z] );
    (void) get_real_argument( 0.0, &tolerance );

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != VIO_OK )
        return( 1 );

    dim_names = get_volume_dimension_names( volume );

    geodesic = create_volume( VIO_N_DIMENSIONS, dim_names, NC_BYTE, FALSE,
                              0.0, 0.0 );

    delete_dimension_names( volume, dim_names );

    get_volume_sizes( volume, sizes );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        if( geo_sizes[dim] < 1 )
            geo_sizes[dim] = sizes[dim] + 1;
    }

    set_volume_sizes( geodesic, geo_sizes );
    alloc_volume_data( geodesic );

    volume_transform = get_voxel_to_world_transform( volume );

    make_identity_transform( &new_to_old );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        Transform_elem( new_to_old, dim, dim ) = (VIO_Real) (sizes[dim]+1) /
                                                 (VIO_Real) geo_sizes[dim];
        Transform_elem( new_to_old, dim, 3 ) = 0.5 * (VIO_Real) (sizes[dim]+1) /
                                               (VIO_Real) geo_sizes[dim] - 1.0;
    }

    create_linear_transform( &mod_transform, &new_to_old );
    concat_general_transforms( &mod_transform, volume_transform,
                               &new_transform );
    set_voxel_to_world_transform( geodesic, &new_transform );
    delete_general_transform( &mod_transform );

    max_voxel = get_volume_voxel_max( geodesic );

    label_surface_voxels( volume, geodesic, threshold,
                          max_voxel-1.0, max_voxel, tolerance );

    convert_world_to_voxel( geodesic, x_world, y_world, z_world, voxel );

    find_nearest_voxel( geodesic, voxel, max_voxel-1.0, origin );

    compute_geodesic_volume( geodesic, origin, max_voxel-1.0, max_voxel );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, geodesic, "Geodesic volume\n", NULL );
    
    return( 0 );
}

static  void  label_surface_voxels(
    VIO_Volume    volume,
    VIO_Volume    geodesic,
    VIO_Real      threshold,
    VIO_Real      surface_value,
    VIO_Real      nonsurface_value,
    VIO_Real      tolerance )
{
    int       x, y, z, v3, v4, dim, dx, dy, dz;
    VIO_Real      voxel[VIO_N_DIMENSIONS], x_world, y_world, z_world, label;
    VIO_Real      min_voxel[VIO_N_DIMENSIONS], max_voxel[VIO_N_DIMENSIONS];
    int       sizes[VIO_N_DIMENSIONS], geo_sizes[VIO_N_DIMENSIONS];
    VIO_progress_struct   progress;

    get_volume_sizes( geodesic, geo_sizes );
    get_volume_sizes( volume, sizes );

    initialize_progress_report( &progress, FALSE,
        geo_sizes[VIO_X] * geo_sizes[VIO_Y] * geo_sizes[VIO_Z], "Finding surface voxels" );

    BEGIN_ALL_VOXELS( geodesic, x, y, z, v3, v4 )

        for_less( dx, 0, 2 )
        for_less( dy, 0, 2 )
        for_less( dz, 0, 2 )
        {
            voxel[VIO_X] = (VIO_Real) x - 0.5 + (VIO_Real) dx;
            voxel[VIO_Y] = (VIO_Real) y - 0.5 + (VIO_Real) dy;
            voxel[VIO_Z] = (VIO_Real) z - 0.5 + (VIO_Real) dz;

            convert_voxel_to_world( geodesic, voxel,
                                    &x_world, &y_world, &z_world );
            convert_world_to_voxel( volume, x_world, y_world, z_world, voxel );

            for_less( dim, 0, VIO_N_DIMENSIONS )
            {
                if( dx == 0 && dy == 0 && dz == 0 ||
                    voxel[dim] < min_voxel[dim] )
                {
                    min_voxel[dim] = voxel[dim];
                }
                if( dx == 0 && dy == 0 && dz == 0 ||
                    voxel[dim] > max_voxel[dim] )
                {
                    max_voxel[dim] = voxel[dim];
                }
            }
        }

        for_less( dim, 0, VIO_N_DIMENSIONS )
        {
            min_voxel[dim] -= tolerance;
            max_voxel[dim] += tolerance;
        }

        if( volume_range_contains_threshold( volume, min_voxel, max_voxel,
                                             threshold ) )
        {
            label = surface_value;
        }
        else
            label = nonsurface_value;

        set_volume_voxel_value( geodesic, x, y, z, 0, 0, (VIO_Real) label );

        update_progress_report( &progress,
                                z + 1 + geo_sizes[VIO_Z] * (y + x * geo_sizes[VIO_Y]) );

    END_ALL_VOXELS

    terminate_progress_report( &progress );
}

static  void  find_nearest_voxel(
    VIO_Volume   volume,
    VIO_Real     voxel[],
    VIO_Real     value,
    int      nearest[] )
{
    int      dim, face, d, origin[VIO_N_DIMENSIONS], sizes[VIO_N_DIMENSIONS];
    int      dist, max_dist, x, y, z;
    int      limits[2][VIO_N_DIMENSIONS];
    VIO_BOOL  found;

    get_volume_sizes( volume, sizes );

    max_dist = 0;
    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        origin[dim] = VIO_ROUND( voxel[dim] );
        if( origin[dim] < 0 )
            origin[dim] = 0;
        else if( origin[dim] >= sizes[dim] )
            origin[dim] = sizes[dim]-1;

        if( origin[dim] > max_dist )
            max_dist = origin[dim];
        if( sizes[dim] - 1 - origin[dim] > max_dist )
            max_dist = sizes[dim] - 1 - origin[dim];
    }

    found = FALSE;
    dist = 0;
    while( dist <= max_dist && !found )
    {
        for_less( dim, 0, VIO_N_DIMENSIONS )
        {
            for_less( face, 0, 2 )
            {
                for_less( d, 0, VIO_N_DIMENSIONS )
                {
                    limits[0][d] = origin[d] - dist;
                    limits[1][d] = origin[d] + dist;
                }

                limits[face][d] = limits[1-face][d];

                for_less( d, 0, VIO_N_DIMENSIONS )
                {
                    if( limits[0][d] < 0 )
                        limits[0][d] = 0;
                    if( limits[1][d] >= sizes[d] )
                        limits[1][d] = sizes[d]-1;
                }

                for_inclusive( x, limits[0][0], limits[1][0] )
                for_inclusive( y, limits[0][1], limits[1][1] )
                for_inclusive( z, limits[0][2], limits[1][2] )
                {
                    if( !found &&
                        get_volume_real_value( volume, x, y, z, 0, 0 ) == value)
                    {
                        found = TRUE;
                        nearest[0] = x;
                        nearest[1] = y;
                        nearest[2] = z;
                    }
                }

                if( found ) break;
            }

            if( found ) break;
        }

        ++dist;
    }
}

static  VIO_BOOL  volume_range_contains_threshold(
    VIO_Volume   volume,
    VIO_Real     min_voxel[],
    VIO_Real     max_voxel[],
    VIO_Real     threshold )
{
    int      dim, sizes[VIO_N_DIMENSIONS];
    VIO_Real     value, x, y, z, voxel[VIO_N_DIMENSIONS];
    VIO_BOOL  x_is_int, y_is_int, z_is_int, above, below;

    get_volume_sizes( volume, sizes );

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        if( min_voxel[dim] < -1.0 )
            min_voxel[dim] = -1.0;
        if( max_voxel[dim] > (VIO_Real) sizes[dim] )
            max_voxel[dim] = (VIO_Real) sizes[dim];
    }

    above = FALSE;
    below = FALSE;

    x = min_voxel[VIO_X];
    x_is_int = VIO_IS_INT(x) && x >= 0.0 && (int) x < sizes[0];
    while( TRUE )
    {
        y = min_voxel[VIO_Y];
        y_is_int = VIO_IS_INT(y) && y >= 0.0 && (int) y < sizes[1];
        while( TRUE )
        {
            z = min_voxel[VIO_Z];
            z_is_int = VIO_IS_INT(z) && z >= 0.0 && (int) z < sizes[2];
            while( TRUE )
            {
                if( x_is_int && y_is_int && z_is_int )
                {
                    value = get_volume_real_value( volume,
                                                   (int) x, (int) y, (int) z,
                                                   0, 0 );
                }
                else
                {
                    voxel[VIO_X] = x;
                    voxel[VIO_Y] = y;
                    voxel[VIO_Z] = z;
                    (void) evaluate_volume( volume, voxel, NULL, 0, FALSE,
                                            0.0, &value, NULL, NULL );
                }

                if( value == threshold )
                    return( TRUE );
                else if( value < threshold )
                {
                    if( above )
                        return( TRUE );
                    below = TRUE;
                }
                else
                {
                    if( below )
                        return( TRUE );
                    above = TRUE;
                }

                if( z >= max_voxel[VIO_Z] )
                    break;

                z += 1.0;
                if( !VIO_IS_INT(z) )
                    z = (VIO_Real) (int) z;
                if( z > max_voxel[VIO_Z] )
                {
                    z = max_voxel[VIO_Z];
                    z_is_int = VIO_IS_INT(z) && z >= 0.0 && (int) z < sizes[2];
                }
                else
                    z_is_int = FALSE;
            }

            if( y >= max_voxel[VIO_Y] )
                break;

            y += 1.0;
            if( !VIO_IS_INT(y) )
                y = (VIO_Real) (int) y;
            if( y > max_voxel[VIO_Y] )
            {
                y = max_voxel[VIO_Y];
                y_is_int = VIO_IS_INT(y);
                y_is_int = VIO_IS_INT(y) && y >= 0.0 && (int) y < sizes[1];
            }
            else
                y_is_int = FALSE;
        }

        if( x >= max_voxel[VIO_X] )
            break;

        x += 1.0;
        if( !VIO_IS_INT(x) )
            x = (VIO_Real) (int) x;
        if( x > max_voxel[VIO_X] )
        {
            x = max_voxel[VIO_X];
            x_is_int = VIO_IS_INT(x) && x >= 0.0 && (int) x < sizes[0];
        }
        else
            x_is_int = FALSE;
    }

    return( FALSE );
}

typedef struct
{
    int   voxel[VIO_N_DIMENSIONS];
} xyz_struct;

static  void  compute_geodesic_volume(
    VIO_Volume   volume,
    int      origin[],
    VIO_Real     surface_value,
    VIO_Real     nonsurface_value )
{
    int                           dim, dist;
    int                           start[VIO_N_DIMENSIONS], end[VIO_N_DIMENSIONS];
    int                           sizes[VIO_N_DIMENSIONS];
    int                           x, y, z;
    QUEUE_STRUCT( xyz_struct )    queue;
    xyz_struct                    entry;
    VIO_Real                          current;

    get_volume_sizes( volume, sizes );

    INITIALIZE_QUEUE( queue );

    for_less( dim, 0, VIO_N_DIMENSIONS )
        entry.voxel[dim] = origin[dim];

    dist = 0;
    set_volume_real_value( volume, origin[VIO_X], origin[VIO_Y], origin[VIO_Z], 0, 0, 0.0 );
    INSERT_IN_QUEUE( queue, entry );

    while( !IS_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_QUEUE( queue, entry );

        dist = 1 + (int) get_volume_real_value( volume, entry.voxel[0],
                                                entry.voxel[1],
                                                entry.voxel[2], 0, 0 );

        for_less( dim, 0, VIO_N_DIMENSIONS )
        {
            if( entry.voxel[dim] == 0 )
                start[dim] = entry.voxel[dim];
            else
                start[dim] = entry.voxel[dim] - 1;

            if( entry.voxel[dim] == sizes[dim]-1 )
                end[dim] = entry.voxel[dim];
            else
                end[dim] = entry.voxel[dim] + 1;
        }

        for_inclusive( x, start[VIO_X], end[VIO_X] )
        for_inclusive( y, start[VIO_Y], end[VIO_Y] )
        for_inclusive( z, start[VIO_Z], end[VIO_Z] )
        {
            current = get_volume_real_value( volume, x, y, z, 0, 0 );

            if( current != nonsurface_value && dist < (int) current )
            {
                set_volume_real_value( volume, x, y, z, 0, 0, (VIO_Real) dist );
                entry.voxel[0] = x;
                entry.voxel[1] = y;
                entry.voxel[2] = z;
                INSERT_IN_QUEUE( queue, entry );
            }
        }
    }
}
