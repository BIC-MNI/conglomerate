
#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  label_surface_voxels(
    Volume    volume,
    Volume    geodesic,
    Real      threshold,
    Real      surface_value,
    Real      nonsurface_value );

private  void  find_nearest_voxel(
    Volume   volume,
    Real     voxel[],
    Real     value,
    int      nearest[] );

private  BOOLEAN  volume_range_contains_threshold(
    Volume   volume,
    Real     min_voxel[],
    Real     max_voxel[],
    Real     threshold );

private  void  compute_geodesic_volume(
    Volume   volume,
    int      origin[],
    Real     surface_value );

private  void  usage(
    char   executable[] )
{
    STRING   usage_str = "\n\
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
    STRING              input_filename, output_filename, *dim_names;
    Volume              volume, geodesic;
    int                 dim, origin[N_DIMENSIONS];
    int                 sizes[N_DIMENSIONS], geo_sizes[N_DIMENSIONS];
    Real                threshold, x_world, y_world, z_world;
    Real                voxel[N_DIMENSIONS], max_voxel;
    Transform           new_to_old;
    General_transform   *volume_transform, mod_transform, new_transform;

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

    (void) get_int_argument( -1, &geo_sizes[X] );
    (void) get_int_argument( -1, &geo_sizes[Y] );
    (void) get_int_argument( -1, &geo_sizes[Z] );

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    dim_names = get_volume_dimension_names( volume );

    geodesic = create_volume( N_DIMENSIONS, dim_names, NC_BYTE, FALSE,
                              0.0, 0.0 );

    delete_dimension_names( volume, dim_names );

    get_volume_sizes( volume, sizes );

    for_less( dim, 0, N_DIMENSIONS )
    {
        if( geo_sizes[dim] < 1 )
            geo_sizes[dim] = sizes[dim] + 1;
    }

    set_volume_sizes( geodesic, geo_sizes );
    alloc_volume_data( geodesic );

    volume_transform = get_voxel_to_world_transform( volume );

    make_identity_transform( &new_to_old );

    for_less( dim, 0, N_DIMENSIONS )
    {
        Transform_elem( new_to_old, dim, dim ) = (Real) (geo_sizes[dim]+1) /
                                                 (Real) sizes[dim];
        Transform_elem( new_to_old, dim, 3 ) = 0.5 * (Real) (geo_sizes[dim]+1) /
                                               (Real) sizes[dim] - 1.0;
    }

    create_linear_transform( &mod_transform, &new_to_old );
    concat_general_transforms( &mod_transform, volume_transform,
                               &new_transform );
    set_voxel_to_world_transform( geodesic, &new_transform );
    delete_general_transform( &mod_transform );

    max_voxel = get_volume_voxel_max( geodesic );

    label_surface_voxels( volume, geodesic, threshold,
                          max_voxel-1.0, max_voxel );

    convert_world_to_voxel( geodesic, x_world, y_world, z_world, voxel );

    find_nearest_voxel( geodesic, voxel, max_voxel-1.0, origin );

    compute_geodesic_volume( geodesic, origin, max_voxel-1.0 );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, geodesic, "Geodesic volume\n", NULL );
    
    return( 0 );
}

private  void  label_surface_voxels(
    Volume    volume,
    Volume    geodesic,
    Real      threshold,
    Real      surface_value,
    Real      nonsurface_value )
{
    int       x, y, z, v3, v4, dim, dx, dy, dz;
    Real      voxel[N_DIMENSIONS], x_world, y_world, z_world, label;
    Real      min_voxel[N_DIMENSIONS], max_voxel[N_DIMENSIONS];
    int       sizes[N_DIMENSIONS], geo_sizes[N_DIMENSIONS];

    get_volume_sizes( geodesic, geo_sizes );
    get_volume_sizes( volume, sizes );

    BEGIN_ALL_VOXELS( geodesic, x, y, z, v3, v4 )

        for_less( dx, 0, 2 )
        for_less( dy, 0, 2 )
        for_less( dz, 0, 2 )
        {
            voxel[X] = (Real) x - 0.5 + (Real) dx;
            voxel[Y] = (Real) y - 0.5 + (Real) dy;
            voxel[Z] = (Real) z - 0.5 + (Real) dz;

            convert_voxel_to_world( geodesic, voxel,
                                    &x_world, &y_world, &z_world );
            convert_world_to_voxel( volume, x_world, y_world, z_world, voxel );

            for_less( dim, 0, N_DIMENSIONS )
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

        if( volume_range_contains_threshold( volume, min_voxel, max_voxel,
                                             threshold ) )
        {
            label = surface_value;
        }
        else
            label = nonsurface_value;

        set_volume_voxel_value( geodesic, x, y, z, 0, 0, (Real) label );

    END_ALL_VOXELS
}

private  void  find_nearest_voxel(
    Volume   volume,
    Real     voxel[],
    Real     value,
    int      nearest[] )
{
    int      dim, face, d, origin[N_DIMENSIONS], sizes[N_DIMENSIONS];
    int      dist, max_dist, x, y, z;
    int      limits[2][N_DIMENSIONS];
    BOOLEAN  found;

    get_volume_sizes( volume, sizes );

    max_dist = 0;
    for_less( dim, 0, N_DIMENSIONS )
    {
        origin[dim] = ROUND( voxel[dim] );
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
        for_less( dim, 0, N_DIMENSIONS )
        {
            for_less( face, 0, 2 )
            {
                for_less( d, 0, N_DIMENSIONS )
                {
                    limits[0][d] = origin[d] - dist;
                    limits[1][d] = origin[d] + dist;
                }

                limits[face][d] = limits[1-face][d];

                for_less( d, 0, N_DIMENSIONS )
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

private  BOOLEAN  volume_range_contains_threshold(
    Volume   volume,
    Real     min_voxel[],
    Real     max_voxel[],
    Real     threshold )
{
    int      dim, sizes[N_DIMENSIONS];
    Real     value, x, y, z, voxel[N_DIMENSIONS];
    BOOLEAN  x_is_int, y_is_int, z_is_int, above, below;

    get_volume_sizes( volume, sizes );

    for_less( dim, 0, N_DIMENSIONS )
    {
        if( min_voxel[dim] < -0.5 )
            min_voxel[dim] = -0.5;
        if( max_voxel[dim] > (Real) sizes[dim] - 0.5 )
            max_voxel[dim] = (Real) sizes[dim] - 0.5;
    }

    above = FALSE;
    below = FALSE;

    x = min_voxel[X];
    x_is_int = IS_INT(x);
    while( TRUE )
    {
        y = min_voxel[Y];
        y_is_int = IS_INT(y);
        while( TRUE )
        {
            z = min_voxel[Z];
            z_is_int = IS_INT(z);
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
                    voxel[X] = x;
                    voxel[Y] = y;
                    voxel[Z] = z;
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

                if( z == max_voxel[Z] )
                    break;

                z += 1.0;
                if( !IS_INT(z) )
                    z = (Real) (int) z;
                if( z > max_voxel[Z] )
                {
                    z = max_voxel[Z];
                    z_is_int = IS_INT(z);
                }
                else
                    z_is_int = FALSE;
            }

            if( y == max_voxel[Y] )
                break;

            y += 1.0;
            if( !IS_INT(y) )
                y = (Real) (int) y;
            if( y > max_voxel[Y] )
            {
                y = max_voxel[Y];
                y_is_int = IS_INT(y);
            }
            else
                y_is_int = FALSE;
        }

        if( x == max_voxel[X] )
            break;

        x += 1.0;
        if( !IS_INT(x) )
            x = (Real) (int) x;
        if( x > max_voxel[X] )
        {
            x = max_voxel[X];
            x_is_int = IS_INT(x);
        }
        else
            x_is_int = FALSE;
    }

    return( FALSE );
}

typedef struct
{
    int   voxel[N_DIMENSIONS];
} xyz_struct;

private  void  compute_geodesic_volume(
    Volume   volume,
    int      origin[],
    Real     surface_value )
{
    int                           dim, dist;
    int                           start[N_DIMENSIONS], end[N_DIMENSIONS];
    int                           sizes[N_DIMENSIONS];
    int                           x, y, z;
    QUEUE_STRUCT( xyz_struct )    queue;
    xyz_struct                    entry;

    get_volume_sizes( volume, sizes );

    INITIALIZE_QUEUE( queue );

    for_less( dim, 0, N_DIMENSIONS )
        entry.voxel[dim] = origin[dim];

    dist = 0;
    set_volume_real_value( volume, origin[X], origin[Y], origin[Z], 0, 0, 0.0 );
    INSERT_IN_QUEUE( queue, entry );

    while( !IS_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_QUEUE( queue, entry );

        dist = 1 + (int) get_volume_real_value( volume, entry.voxel[0],
                                                entry.voxel[1],
                                                entry.voxel[2], 0, 0 );

        for_less( dim, 0, N_DIMENSIONS )
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

        for_inclusive( x, start[X], end[X] )
        for_inclusive( y, start[Y], end[Y] )
        for_inclusive( z, start[Z], end[Z] )
        {
            if( dist < (int) get_volume_real_value( volume, x, y, z, 0, 0 ))
            {
                set_volume_real_value( volume, x, y, z, 0, 0, (Real) dist );
                entry.voxel[0] = x;
                entry.voxel[1] = y;
                entry.voxel[2] = z;
                INSERT_IN_QUEUE( queue, entry );
            }
        }
    }
}
