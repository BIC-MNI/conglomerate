#include  <internal_volume_io.h>
#include  <bicpl.h>

private  int  dilate(
    Volume    volume,
    Volume    out_volume,
    int       n_dirs,
    int       dx[],
    int       dy[],
    int       dz[],
    int       value );

private  int  erode(
    Volume    volume,
    int       n_dirs,
    int       dx[],
    int       dy[],
    int       dz[] );

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s input.mnc output.mnc\n\
            [6|26]\n\
\n\
     No help yet.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_filename, output_filename;
    Volume               volume, out_volume;
    int                  v1, v2, v3, v4, v5, n_changed, value, n_neighs;
    int                  n_dirs, *dx, *dy, *dz, max_label;
    int                  total_added, n_eroded;
    Real                 voxel;
    Neighbour_types      connectivity;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 26, &n_neighs );
    (void) get_int_argument( 256, &max_label );

    switch( n_neighs )
    {
    case 6:   connectivity = FOUR_NEIGHBOURS;  break;
    case 26:   connectivity = EIGHT_NEIGHBOURS;  break;
    default:  print_error( "# neighs must be 6 or 26.\n" );  return( 1 );
    }

    n_dirs = get_3D_neighbour_directions( connectivity, &dx, &dy, &dz );

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      NULL ) != OK )
        return( 1 );

    out_volume = copy_volume( volume );

    value = 2;
    do
    {
        if( value > 2 )
        {
            BEGIN_ALL_VOXELS( volume, v1, v2, v3, v4, v5 )
                voxel = get_volume_voxel_value( out_volume, v1, v2, v3, v4, v5);
                set_volume_voxel_value( volume, v1, v2, v3, v4, v5, voxel );
            END_ALL_VOXELS
        }

        n_changed = dilate( volume, out_volume, n_dirs, dx, dy, dz,
                            value );
        total_added = n_changed;
        print( "Dilated %d: %d\n", value, n_changed );
        ++value;
    }
    while( n_changed > 0 && value < max_label );

    delete_volume( volume );

    n_eroded = erode( out_volume, n_dirs, dx, dy, dz );

    print( "Initially changed: %d\n", total_added );
    print( "Final changed: %d\n", total_added - n_eroded );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, out_volume, input_filename,
                                   "Dilated\n", NULL );

    return( 0 );
}

private  int  dilate(
    Volume    volume,
    Volume    out_volume,
    int       n_dirs,
    int       dx[],
    int       dy[],
    int       dz[],
    int       value )
{
    int   dir, n_changed, v0, v1, v2, v3, v4, sizes[N_DIMENSIONS];
    int   t0, t1, t2;

    get_volume_sizes( volume, sizes );
    n_changed = 0;

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )
        if( get_volume_real_value( volume,v0,v1,v2,0,0 ) != 0.0 )
            continue;

        for_less( dir, 0, n_dirs )
        {
            t0 = v0 + dx[dir];
            t1 = v1 + dy[dir];
            t2 = v2 + dz[dir];

            if( t0 >= 0 && t0 < sizes[0] &&
                t1 >= 0 && t1 < sizes[1] &&
                t2 >= 0 && t2 < sizes[2] &&
                get_volume_real_value(volume,t0,t1,t2,0,0) > 0.0 )
            {
                break;
            }
        }

        if( dir < n_dirs )
        {
            set_volume_real_value( out_volume,v0,v1,v2,0,0, (Real) value );
            ++n_changed;
        }

    END_ALL_VOXELS

    return( n_changed );
}

private  BOOLEAN  can_delete_voxel(
    Volume    volume,
    int       v0,
    int       v1,
    int       v2,
    int       n_dirs,
    int       dx[],
    int       dy[],
    int       dz[] )
{
    return( TRUE );
}

typedef  struct
{
    unsigned short  v0;
    unsigned short  v1;
    unsigned short  v2;
} voxel_struct;

private  int  erode(
    Volume    volume,
    int       n_dirs,
    int       dx[],
    int       dy[],
    int       dz[] )
{
    int           v0, v1, v2, v3, v4, t0, t1, t2;
    int           dir, n_eroded, sizes[N_DIMENSIONS];
    Real          value, priority;
    PRIORITY_QUEUE_STRUCT( voxel_struct )   queue;
    voxel_struct                            entry;
    Smallest_int  ***in_queue;

    n_eroded = 0;
    get_volume_sizes( volume, sizes );

    ALLOC3D( in_queue, sizes[0], sizes[1], sizes[2] );

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )
        in_queue[v0][v1][v2] = FALSE;
    END_ALL_VOXELS

    INITIALIZE_PRIORITY_QUEUE( queue );

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )
        if( get_volume_real_value(volume,v0,v1,v2,0,0) <= 1.0 )
            continue;

        for_less( dir, 0, n_dirs )
        {
            t0 = v0 + dx[dir];
            t1 = v1 + dy[dir];
            t2 = v2 + dz[dir];

            if( t0 < 0 || t0 >= sizes[0] ||
                t1 < 0 || t1 >= sizes[1] ||
                t2 < 0 || t2 >= sizes[2] ||
                get_volume_real_value(volume,t0,t1,t2,0,0) == 0.0 )
            {
                break;
            }
        }

        if( dir < n_dirs )
        {
            value = get_volume_real_value( volume, v0, v1, v2, 0, 0 );
            entry.v0 = (unsigned short) v0;
            entry.v1 = (unsigned short) v1;
            entry.v2 = (unsigned short) v2;
            INSERT_IN_PRIORITY_QUEUE( queue, entry, value );
            in_queue[v0][v1][v2] = TRUE;
        }

    END_ALL_VOXELS

    while( !IS_PRIORITY_QUEUE_EMPTY( queue ) )
    {
        REMOVE_FROM_PRIORITY_QUEUE( queue, entry, priority );

        v0 = (int) entry.v0;
        v1 = (int) entry.v1;
        v2 = (int) entry.v2;
        in_queue[v0][v1][v2] = FALSE;

        if( can_delete_voxel( volume, v0, v1, v2, n_dirs, dx, dy, dz ) )
        {
            set_volume_real_value( volume, v0, v1, v2, 0, 0, 0.0 );
            ++n_eroded;

            for_less( dir, 0, n_dirs )
            {
                t0 = v0 + dx[dir];
                t1 = v1 + dy[dir];
                t2 = v2 + dz[dir];

                if( t0 >= 0 && t0 < sizes[0] &&
                    t1 >= 0 && t1 < sizes[1] &&
                    t2 >= 0 && t2 < sizes[2] &&
                    !in_queue[t0][t1][t2] )
                {
                    value = get_volume_real_value(volume,t0,t1,t2,0,0);
                    if( value > 1.0 )
                    {
                        entry.v0 = (unsigned short) t0;
                        entry.v1 = (unsigned short) t1;
                        entry.v2 = (unsigned short) t2;
                        INSERT_IN_PRIORITY_QUEUE( queue, entry, value );
                        in_queue[t0][t1][t2] = TRUE;
                    }
                }
            }
        }
    }

    DELETE_PRIORITY_QUEUE( queue );
    FREE3D( in_queue );

    return( n_eroded );
}
