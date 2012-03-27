#include  <volume_io.h>
#include  <bicpl.h>

private  void  chamfer_volume(
    Volume   volume );
private  void  peel_volume(
    Volume   volume,
    Real     distance );

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s input.mnc output.mnc  distance\n\
\n\
\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename, mask_filename;
    STRING               *dim_names;
    Real                 min_mask, max_mask, distance, value;
    BOOLEAN              mask_volume_present;
    Volume               volume, mask_volume;
    int                  i, n_dilations, n_neighs, n_changed;
    int                  range_changed[2][N_DIMENSIONS];
    int                  x, y, z, sizes[N_DIMENSIONS];
    Neighbour_types      connectivity;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &distance ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    for_less( z, 0, sizes[Z] )
    {
        value = get_volume_real_value( volume, x, y, z, 0, 0 );

        if( value > 0.0 )
            set_volume_real_value( volume, x, y, z, 0, 0, 0.0 );
        else
            set_volume_real_value( volume, x, y, z, 0, 0, 255.0 );
    }

    chamfer_volume( volume );

    peel_volume( volume, distance );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, input_filename,
                                   "Dilated\n",
                                   NULL );

    return( 0 );
}

private  void  chamfer_volume(
    Volume   volume )
{
    int      dx, dy, dz, tx, ty, tz, x, y, z, sizes[N_DIMENSIONS];
    int      n_changed, iter;
    Real     neigh, current, min_neigh;

    get_volume_sizes( volume, sizes );
    iter = 0;

    do
    {
        n_changed = 0;

        for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            current = get_volume_real_value( volume, x, y, z, 0, 0 );

            if( current != (Real) iter )
                continue;

            for_inclusive( dx, -1, 1 )
            for_inclusive( dy, -1, 1 )
            for_inclusive( dz, -1, 1 )
            {
                tx = x + dx;
                ty = y + dy;
                tz = z + dz;

                if( tx >= 0 && tx < sizes[X] &&
                    ty >= 0 && ty < sizes[Y] &&
                    tz >= 0 && tz < sizes[Z] )
                {
                    neigh = get_volume_real_value( volume, tx, ty, tz, 0, 0 );
                    if( neigh == 255.0 )
                    {
                        set_volume_real_value( volume, tx, ty, tz, 0, 0,
                                               current + 1.0 );
                        ++n_changed;
                    }
                }
            }
        }

        ++iter;
        print( "Chamfer Iter: %d   N changed: %d\n", iter, n_changed );
    }
    while( n_changed > 0 );
}

private  void  get_3by3(
    Volume   volume,
    int      sizes[],
    int      x,
    int      y,
    int      z,
    int      cube[3][3][3] )
{
    int   dx, dy, dz, tx, ty, tz;

    for_inclusive( dx, -1, 1 )
    for_inclusive( dy, -1, 1 )
    for_inclusive( dz, -1, 1 )
    {
        tx = x + dx;
        ty = y + dy;
        tz = z + dz;

        if( tx >= 0 && tx < sizes[X] &&
            ty >= 0 && ty < sizes[Y] &&
            tz >= 0 && tz < sizes[Z] &&
            get_volume_real_value(volume,tx,ty,tz,0,0) != 255.0 )
        {
            cube[dx+1][dy+1][dz+1] = 1;
        }
        else
            cube[dx+1][dy+1][dz+1] = 0;
    }
}

private  void  count_connected(
    int       cube[3][3][3],
    int       *n_full,
    int       *n_empty )
{
#ifndef NO
    static int  dx[] = { 1, 0, 0, -1, 0, 0 };
    static int  dy[] = { 0, 1, 0, 0, -1, 0 };
    static int  dz[] = { 0, 0, 1, 0, 0, -1 };
#else
    static int  dx[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         0, 0, 0, 0, 0, 0, 0, 0,
                         -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    static int  dy[] = { 1, 1, 1, 0, 0, 0, -1, -1, -1,
                         1, 1, 1, 0, 0, -1, -1, -1,
                         1, 1, 1, 0, 0, 0, -1, -1, -1 };
    static int  dz[] = { 1, 0, -1, 1, 0, -1, 1, 0, -1,
                         1, 0, -1, 1, -1, 1, 0, -1,
                         1, 0, -1, 1, 0, -1, 1, 0, -1 };
#endif
    int         x, y, z, tx, ty, tz, nx, ny, nz;
    int         dir, offset, new_id;
    int         queue[27][3], queue_head, queue_tail;

    for_less( x, 0, 3 )
    for_less( y, 0, 3 )
    for_less( z, 0, 3 )
    {
        if( cube[x][y][z] > 0 )
            cube[x][y][z] = 100;
        else
            cube[x][y][z] = 200;
    }

    *n_full = 0;
    *n_empty = 0;
    for_less( x, 0, 3 )
    for_less( y, 0, 3 )
    for_less( z, 0, 3 )
    {
        if( cube[x][y][z] != 100 && cube[x][y][z] != 200 )
            continue;

        offset = cube[x][y][z];
        if( offset == 100 )
        {
            ++(*n_full);
            new_id = offset + (*n_full);
        }
        else
        {
            ++(*n_empty);
            new_id = offset + (*n_empty);
        }
        
        cube[x][y][z] = new_id;
        queue[0][0] = x;
        queue[0][1] = x;
        queue[0][2] = x;
        queue_head = 0;
        queue_tail = 1;

        while( queue_head < queue_tail )
        {
            nx = queue[queue_head][0];
            ny = queue[queue_head][1];
            nz = queue[queue_head][2];
            ++queue_head;
            for_less( dir, 0, SIZEOF_STATIC_ARRAY( dx ) )
            {
                tx = nx + dx[dir];
                ty = ny + dy[dir];
                tz = nz + dz[dir];

                if( tx >= 0 && tx < 3 &&
                    ty >= 0 && ty < 3 &&
                    tz >= 0 && tz < 3 &&
                    cube[tx][ty][tz] == offset )
                {
                    cube[tx][ty][tz] = new_id;
                    queue[queue_tail][0] = tx;
                    queue[queue_tail][1] = ty;
                    queue[queue_tail][2] = tz;
                    ++queue_tail;
                }
            }
        }
    }

    for_less( x, 0, 3 )
    for_less( y, 0, 3 )
    for_less( z, 0, 3 )
    {
        if( cube[x][y][z] >= 200 )
            cube[x][y][z] = 0;
        else
            cube[x][y][z] = 1;
    }
}

private  BOOLEAN  check_connectivity(
    Volume   volume,
    int      sizes[],
    int      x,
    int      y,
    int      z )
{
    int       n_full_before, n_full_after;
    int       n_empty_before, n_empty_after;
    int       cube[3][3][3];

    get_3by3( volume, sizes, x, y, z, cube );

    count_connected( cube, &n_full_before, &n_empty_before );

    cube[1][1][1] = 0;

    count_connected( cube, &n_full_after, &n_empty_after );

    return( n_full_before == n_full_after );
}

private  void  peel_volume(
    Volume   volume,
    Real     distance )
{
    int      x, y, z, sizes[N_DIMENSIONS];
    int      n_changed, iter;
    Real     neigh, current, min_neigh;
    BOOLEAN  can_remove;

    get_volume_sizes( volume, sizes );
    iter = 0;

    do
    {
        n_changed = 0;

        for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            current = get_volume_real_value( volume, x, y, z, 0, 0 );

            if( current <= distance || current == 255.0 )
                continue;

            can_remove = check_connectivity( volume, sizes, x, y, z );

            if( can_remove )
            {
                set_volume_real_value( volume, x, y, z, 0, 0, 255.0 );
                ++n_changed;
            }
        }

        ++iter;
        print( "Peel Iter: %d   N changed: %d\n", iter, n_changed );
    }
    while( n_changed > 0 );
}
