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
        print( "%d: %d\n", value, n_changed );
        ++value;
    }
    while( n_changed > 0 && value < max_label );

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
