#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  chamfer_volume(
    Volume   volume,
    int      dist );
private  void  expand_layer(
    Volume             volume,
    int                iter,
    int                n_sets,
    int                n_in_set[],
    unsigned long      *sets[] );
private  int  make_lists_of_required_voxels(
    Volume          volume,
    int             dist,
    int             *n_in_set[],
    unsigned long   ***sets );

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
    int                  i, n_dilations, n_neighs, n_changed, iter;
    int                  range_changed[2][N_DIMENSIONS];
    int                  x, y, z, sizes[N_DIMENSIONS];
    int                  n_sets, *n_in_set;
    unsigned long        **sets;
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

    chamfer_volume( volume, ROUND( distance ) + 1 );

    n_sets = make_lists_of_required_voxels( volume, ROUND(distance),
                                            &n_in_set, &sets );

    for_inclusive( iter, 1, ROUND(distance) )
    {
        expand_layer( volume, iter, n_sets, n_in_set, sets );
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, input_filename,
                                   "Dilated\n",
                                   NULL );

    return( 0 );
}

private  void  chamfer_volume(
    Volume   volume,
    int      dist )
{
    int      dx, dy, dz, tx, ty, tz, x, y, z, sizes[N_DIMENSIONS];
    int      n_changed, which_dist;
    Real     neigh, current, min_neigh;

    get_volume_sizes( volume, sizes );

    for_less( which_dist, 0, dist )
    {
        n_changed = 0;

        for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            current = get_volume_real_value( volume, x, y, z, 0, 0 );

            if( current != (Real) which_dist )
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

        print( "Chamfer Iter: %d   N changed: %d\n", which_dist+1, n_changed );

        if( n_changed == 0 )
            break;
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

private  void  get_27_neighbours(
    Volume          volume,
    int             x,
    int             y,
    int             z,
    int             neighbours[3][3][3] )
{
    int      dx, dy, dz, tx, ty, tz, sizes[N_DIMENSIONS];
    Real     value;

    get_volume_sizes( volume, sizes );

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
            value = get_volume_real_value( volume, tx, ty, tz, 0, 0 );
            neighbours[dx+1][dy+1][dz+1] = ROUND( value );
        }
        else
        {
            neighbours[dx+1][dy+1][dz+1] = 255;
        }
    }
}

private  BOOLEAN  check_connectivity(
    Volume   volume,
    int      sizes[],
    int      x,
    int      y,
    int      z )
{
    int       n_full_before, n_full_after, dx, dy, dz;
    int       n_empty_before, n_empty_after;
    int       cube[3][3][3];

    get_27_neighbours( volume, x, y, z, cube );

    for_less( dx, 0, 3 )
    for_less( dy, 0, 3 )
    for_less( dz, 0, 3 )
    {
        if( cube[dx][dy][dz] != 0 )
            cube[dx][dy][dz] = 0;
        else
            cube[dx][dy][dz] = 1;
    }

    count_connected( cube, &n_full_before, &n_empty_before );

    cube[1][1][1] = 1;

    count_connected( cube, &n_full_after, &n_empty_after );

    return( n_full_before == n_full_after );
}

private  int  get_set_of_possibles(
    Volume          volume,
    int             dist,
    int             x,
    int             y,
    int             z,
    unsigned long   set[] )
{
    int    tx, ty, tz, neighbours[3][3][3];
    int    sizes[N_DIMENSIONS], dx, dy, dz, nx, ny, nz;
    int    ***box, ***dist_from_voxel, max_size, iter, n_of_max, max_value;

    get_27_neighbours( volume, x, y, z, neighbours );

    for_less( tx, 0, 3 )
    {
        for_less( ty, 0, 3 )
        {
            for_less( tz, 0, 3 )
            {
                if( neighbours[tx][ty][tz] == 1 )
                    break;
            }
            if( tz < 3 )
                break;
        }
        if( ty < 3 )
            break;
    }

    if( tx == 3 )
        return( 0 );

    max_size = 1 + 2 * (dist+1);
    ALLOC3D( box, max_size, max_size, max_size );
    ALLOC3D( dist_from_voxel, max_size, max_size, max_size );

    get_volume_sizes( volume, sizes );

    for_less( dx, 0, max_size )
    for_less( dy, 0, max_size )
    for_less( dz, 0, max_size )
    {
        tx = x + dx - (dist+1);
        ty = y + dy - (dist+1);
        tz = z + dz - (dist+1);

        if( tx >= 0 && tx < sizes[X] &&
            ty >= 0 && ty < sizes[Y] &&
            tz >= 0 && tz < sizes[Z] )
        {
            box[dx][dy][dz] = (int) get_volume_real_value( volume, tx, ty, tz,
                                                           0, 0 );
        }
        else
        {
            box[dx][dy][dz] = 255;
        }

        if( box[dx][dy][dz] == 0 )
            dist_from_voxel[dx][dy][dz] = 0;
        else
            dist_from_voxel[dx][dy][dz] = 255;
    }

    for_less( iter, 0, dist+1 )
    {
        for_less( tx, 0, max_size )
        for_less( ty, 0, max_size )
        for_less( tz, 0, max_size )
        {
            if( dist_from_voxel[tx][ty][tz] != iter )
                continue;

            for_inclusive( dx, -1, 1 )
            for_inclusive( dy, -1, 1 )
            for_inclusive( dz, -1, 1 )
            {
                nx = tx + dx;
                ny = ty + dy;
                nz = tz + dz;

                if( nx >= 0 && nx < max_size &&
                    ny >= 0 && ny < max_size &&
                    nz >= 0 && nz < max_size &&
                    dist_from_voxel[nx][ny][nz] == 255 )
                {
                    dist_from_voxel[nx][ny][nz] = iter+1;
                }
            }
        }
    }

    max_value = 0;
    n_of_max = 0;
    for_less( tx, 0, max_size )
    for_less( ty, 0, max_size )
    for_less( tz, 0, max_size )
    {
        if( dist_from_voxel[tx][ty][tz] == 0 ||
            dist_from_voxel[tx][ty][tz] == 255 )
            continue;

        if( box[tx][ty][tz] > max_value )
        {
            max_value = box[tx][ty][tz];
            n_of_max = 0;
        }

        if( box[tx][ty][tz] == max_value )
            ++n_of_max;
    }

    if( n_of_max > 0 && max_value < dist+1 )
    {
        n_of_max = 0;
        for_less( tx, 0, max_size )
        for_less( ty, 0, max_size )
        for_less( tz, 0, max_size )
        {
            if( dist_from_voxel[tx][ty][tz] > 0 &&
                dist_from_voxel[tx][ty][tz] < 255 &&
                box[tx][ty][tz] == max_value )
            {
                nx = x + tx - (dist+1);
                ny = y + ty - (dist+1);
                nz = z + tz - (dist+1);

                set[n_of_max] = IJK( nx, ny, nz, sizes[Y], sizes[Z] );
                ++n_of_max;
            }
        }

#ifndef NOT
       { int i;
        print( "\n" );
        print( "Voxel: %d %d %d\n", x, y, z );
        for_less( i, 0, n_of_max )
        {
            unsigned long t, a, b, c;

            t = set[i];
            c = t % sizes[Z];
            t /= sizes[Z];
            b = t % sizes[Y];
            a = t / sizes[Y];
            print( "   %d %d %d\n", a, b, c );
        }
      }
#endif
    }
    else
        n_of_max = 0;

    FREE3D( dist_from_voxel );
    FREE3D( box );

    return( n_of_max );
}

private  int  make_lists_of_required_voxels(
    Volume          volume,
    int             dist,
    int             *n_in_set[],
    unsigned long   ***sets )
{
    int             n_sets, max_size, sizes[N_DIMENSIONS];
    int             i, x, y, z, set_size;
    unsigned  long  *possibles;
    Real            value;
    progress_struct progress;

    *n_in_set = NULL;
    *sets = NULL;
    n_sets = 0;

    get_volume_sizes( volume, sizes );

    max_size = (1 + 2 * dist) * (1 + 2 * dist) * (1 + 2 * dist);
    ALLOC( possibles, max_size );

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Making lists" );
    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    {
        for_less( z, 0, sizes[Z] )
        {
            value = get_volume_real_value( volume, x, y, z, 0, 0 );
            if( value != 0.0 )
                continue;

            set_size = get_set_of_possibles( volume, dist, x, y, z, possibles );

            if( set_size == 0 )
                continue;

            SET_ARRAY_SIZE( *n_in_set, n_sets, n_sets+1, DEFAULT_CHUNK_SIZE );
            SET_ARRAY_SIZE( *sets, n_sets, n_sets+1, DEFAULT_CHUNK_SIZE );
            ALLOC( (*sets)[n_sets], set_size );

            (*n_in_set)[n_sets] = set_size;
            for_less( i, 0, set_size )
                (*sets)[n_sets][i] = possibles[i];
            ++n_sets;
        }

        update_progress_report( &progress, x * sizes[Y] + y + 1 );
    }

    terminate_progress_report( &progress );

    FREE( possibles );

    return( n_sets );
}

private  BOOLEAN  alone_in_list(
    int                n_sets,
    int                n_in_set[],
    unsigned long      *sets[],
    unsigned long      voxel )
{
/*
    int   i, j;

    for_less( i, 0, n_sets )
    {
        for_less( j, 0, n_in_set[i] )
        {
            if( sets[i][j] == voxel )
                return( TRUE );
        }
    }
    return( FALSE );
*/
    int   i;

    for_less( i, 0, n_sets )
    {
        if( n_in_set[i] == 1 && sets[i][0] == voxel )
            return( TRUE );
    }
    return( FALSE );
}

private  void  remove_from_lists(
    int                n_sets,
    int                n_in_set[],
    unsigned long      *sets[],
    unsigned long      voxel )
{
    int   i, j, k;

    for_less( i, 0, n_sets )
    {
        for_less( j, 0, n_in_set[i] )
        {
            if( sets[i][j] == voxel )
                break;
        }

        if( j < n_in_set[i] )
        {
            --n_in_set[i];
            for_less( k, j, n_in_set[i] )
                sets[i][k] = sets[i][k+1];
        }
    }
}

private  void  expand_layer(
    Volume             volume,
    int                iter,
    int                n_sets,
    int                n_in_set[],
    unsigned long      *sets[] )
{
    int      x, y, z, dx, dy, dz, sizes[N_DIMENSIONS];
    int      n_changed, neighbours[3][3][3];
    BOOLEAN  can_remove;

    get_volume_sizes( volume, sizes );

    do
    {
        n_changed = 0;

        for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            get_27_neighbours( volume, x, y, z, neighbours );

            if( neighbours[1][1][1] != iter )
                continue;

            for_less( dx, 0, 3 )
            {
                for_less( dy, 0, 3 )
                {
                    for_less( dz, 0, 3 )
                    {
                        if( neighbours[dx][dy][dz] == 0 )
                            break;
                    }
                    if( dz < 3 )
                        break;
                }
                if( dy < 3 )
                    break;
            }

            if( dx == 3 )
                continue;

            can_remove = check_connectivity( volume, sizes, x, y, z );

            if( can_remove &&
                !alone_in_list( n_sets, n_in_set, sets,
                                IJK( x, y, z, sizes[Y], sizes[Z] ) ) )
            {
                set_volume_real_value( volume, x, y, z, 0, 0, 0.0 );
                remove_from_lists( n_sets, n_in_set, sets,
                                   IJK( x, y, z, sizes[Y], sizes[Z] ) );
                ++n_changed;
            }
        }

        print( "Expand Iter: %d   N changed: %d\n", iter, n_changed );
    }
    while( n_changed > 0 );
}

