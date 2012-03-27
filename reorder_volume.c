#include  <volume_io.h>
#include  <bicpl.h>

private  void  reorder_volume(
    Volume   volume,
    Volume   new_volume,
    int      n_bits,
    int      bit_order[] );

private  void  usage(
    STRING   executable )
{
    STRING   usage_str = "\n\
Usage: %s input.mnc output.mnc bit_index0 bit_index1 ...\n\
\n\
     \n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    int        i, bit_order[32], sizes[N_DIMENSIONS], x, y, z;
    Volume     volume, new_volume;
    STRING     input_filename, output_filename;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    for_less( i, 0, 32 )
    {
        if( !get_int_argument( 0, &bit_order[i] ) )
        {
            usage( argv[0] );
            return( 1 );
        }
    }

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    new_volume = copy_volume_definition( volume,
                                         NC_UNSPECIFIED, FALSE, 0.0, 0.0 );

    get_volume_sizes( volume, sizes );
    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    for_less( z, 0, sizes[Z] )
        set_volume_voxel_value( new_volume, x, y, z, 0, 0, 0.0 );

    reorder_volume( volume, new_volume, 32, bit_order );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, new_volume, NULL, NULL );


    return( 0 );
}

private  BOOLEAN  get_position(
    int     n_bits,
    int     current_bits[],
    int     sizes[],
    int     *x,
    int     *y,
    int     *z,
    int     *bit_index )
{
    if( current_bits[0] +
        current_bits[1] +
        current_bits[2] +
        current_bits[3] +
        current_bits[4] +
        current_bits[5] +
        current_bits[6] +
        current_bits[7] != 1 )
    {
        return( FALSE );
    }

    *bit_index = current_bits[0] * 0 +
                 current_bits[1] * 1 +
                 current_bits[2] * 2 +
                 current_bits[3] * 3 +
                 current_bits[4] * 4 +
                 current_bits[5] * 5 +
                 current_bits[6] * 6 +
                 current_bits[7] * 7;

    *z = (current_bits[8] << 0) +
         (current_bits[9] << 1) +
         (current_bits[10] << 2) +
         (current_bits[11] << 3) +
         (current_bits[12] << 4) +
         (current_bits[13] << 5) +
         (current_bits[14] << 6) +
         (current_bits[15] << 7);

    *y = (current_bits[16] << 0) +
         (current_bits[17] << 1) +
         (current_bits[18] << 2) +
         (current_bits[19] << 3) +
         (current_bits[20] << 4) +
         (current_bits[21] << 5) +
         (current_bits[22] << 6) +
         (current_bits[23] << 7);

    *x = (current_bits[24] << 0) +
         (current_bits[25] << 1) +
         (current_bits[26] << 2) +
         (current_bits[27] << 3) +
         (current_bits[28] << 4) +
         (current_bits[29] << 5) +
         (current_bits[30] << 6) +
         (current_bits[31] << 7);

    return( *x < sizes[X] && *y < sizes[Y] && *z < sizes[Z] && *bit_index < 8 );
}

private  void  output_bit(
    Volume   volume,
    int      sizes[],
    int      which_bit )
{
    int    bit_index, x, y, z, value;

    bit_index = which_bit % 8;
    which_bit = which_bit / 8;
    z = which_bit % sizes[Z];
    which_bit = which_bit / sizes[Z];
    y = which_bit % sizes[Y];
    x = which_bit / sizes[Y];

    value = (int) get_volume_voxel_value( volume, x, y, z, 0, 0 );
    set_volume_voxel_value( volume, x, y, z, 0, 0, (Real) ( value | (1<<bit_index)) );
}


private  void  reorder_volume(
    Volume   volume,
    Volume   new_volume,
    int      n_bits,
    int      bit_order[] )
{
    int       *current_bits, bit, sizes[N_DIMENSIONS];
    int       out_bit_index, value, which, x, y, z, bit_index;
    BOOLEAN   done;

    get_volume_sizes( volume, sizes );

    ALLOC( current_bits, n_bits );
    for_less( bit, 0, n_bits )
        current_bits[bit] = 0;

    current_bits[0] = 1;

    out_bit_index = 0;

    (void) get_position( n_bits, current_bits, sizes, &x, &y, &z, &bit_index);

    while( TRUE )
    {
print( "%d %d %d  %d\n", x, y, z, bit_index );

        value = (int) get_volume_voxel_value( volume, x, y, z, 0, 0 );

        bit = (value & (1 << bit_index)) != 0;

        if( bit )
            output_bit( new_volume, sizes, out_bit_index );

        ++out_bit_index;

        which = n_bits-1;
        while( which >= 0 )
        {
            ++current_bits[bit_order[which]];
            if( current_bits[bit_order[which]] == 1 )
            {
                if( bit_order[which] < 8 )
                {
                    for_less( bit, 0, 8 )
                    {
                        if( bit != bit_order[which] )
                            current_bits[bit] = 0;
                    }
                }

                if( get_position( n_bits, current_bits, sizes,
                                  &x, &y, &z, &bit_index ) )
                    break;
            }

            current_bits[bit_order[which]] = 0;
            --which;
        }

        if( which < 0 )
            break;
    }

    if( out_bit_index += 8 * sizes[X] * sizes[Y] * sizes[Z] )
    {
        print( "n bits error %d %d\n",
                out_bit_index, 8 * sizes[X] * sizes[Y] * sizes[Z] );
    }

    FREE( current_bits );
}
