#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  chamfer_volume(
    Volume   volume );

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
        print( "Iter: %d   N changed: %d\n", iter, n_changed );
    }
    while( n_changed > 0 );
}
