#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING   usage_str = "\n\
Usage: %s input.mnc output.mnc x_width y_width z_width\n\
           [world|voxel] [byte]\n\
\n\
     Box filters a volume with the given voxel widths, or, if [world] specified,\n\
     then in mm widths.  If byte is specified, then a byte volume is created.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    int              x, y, z, x1, y1, z1;
    int              sizes[N_DIMENSIONS], n_voxels;
    float            ***float_ptr, *ptr, weight, value;
    Real             min_value, max_value;
    Volume           volume, new_volume;
    STRING           input_filename, output_filename;
    progress_struct  progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    new_volume = copy_volume_definition( volume, NC_FLOAT, FALSE, 0.0, 0.0 );

    float_ptr = (float ***) new_volume->array.data;

    get_volume_sizes( volume, sizes );

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Zeroing" );

    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    {
        for_less( z, 0, sizes[Z] )
            float_ptr[x][y][z] = (float) 0.0;
        update_progress_report( &progress, x * sizes[Y] + y + 1 );
    }

    terminate_progress_report( &progress );

    get_volume_sizes( volume, sizes );

    n_voxels = sizes[X] * sizes[Y] * sizes[Z];

    initialize_progress_report( &progress, FALSE,
                                sizes[X] * sizes[Y] * sizes[Z], "Filtering" );

    weight = 1.0f;

    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    for_less( z, 0, sizes[Z] )
    {
        value = (float) get_volume_real_value( volume, x, y, z, 0, 0 );

        ptr = &float_ptr[0][0][0];
        for_less( x1, 0, sizes[X] )
        for_less( y1, 0, sizes[Y] )
        {
            ptr[0] += 0.3 * value;
            ptr[1] += 0.4 * value;
            ptr[2] += 0.5 * value;
            ptr[3] += 0.2 * value;
            ptr[4] += 0.1 * value;
            ptr[5] += 0.2 * value;
            ptr[6] += 0.1 * value;
            ptr[7] += 0.1 * value;
            ptr[8] += 0.5 * value;
            ptr[9] += 0.2 * value;
            ptr[10] += 0.5 * value;
            ptr[11] += 0.1 * value;
            ptr[12] += 0.2 * value;
            ptr[13] += 0.5 * value;
            ptr[14] += 0.1 * value;
            ptr[15] += 0.2 * value;
            ptr[16] += 0.1 * value;
            ptr[17] += 0.2 * value;
            ptr[18] += 0.1 * value;
            ptr[19] += 0.1 * value;
            ptr[20] += 0.5 * value;
            ptr[21] += 0.2 * value;
            ptr[22] += 0.5 * value;
            ptr[23] += 0.1 * value;
            ptr[24] += 0.1 * value;
            ptr[25] += 0.1 * value;
            ptr[26] += 0.1 * value;
            ptr[27] += 0.2 * value;
            ptr[28] += 0.1 * value;
            ptr[29] += 0.1 * value;
            ptr[30] += 0.2 * value;
            ptr[31] += 0.1 * value;
            ptr[32] += 0.1 * value;
            ptr[33] += 0.5 * value;
            ptr[34] += 0.2 * value;
            ptr[35] += 0.1 * value;
            ptr[36] += 0.5 * value;
            ptr[37] += 0.5 * value;
            ptr[38] += 0.1 * value;
            ptr[39] += 0.1 * value;
        }

        update_progress_report( &progress,
                                z + 1 + sizes[Z] * (y + x * sizes[Y]) );
    }

    terminate_progress_report( &progress );

    min_value = (Real) float_ptr[0][0][0];
    max_value = (Real) float_ptr[0][0][0];

    initialize_progress_report( &progress, FALSE,
                                sizes[X] * sizes[Y], "Computing Range" );

    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    {
        for_less( z, 0, sizes[Z] )
        {
            if( (Real) float_ptr[x][y][z] < min_value )
                min_value = (Real) float_ptr[x][y][z];
            else if( (Real) float_ptr[x][y][z] > max_value )
                max_value = (Real) float_ptr[x][y][z];
        }

        update_progress_report( &progress,
                                y + 1 + x * sizes[Y] );
    }


    set_volume_real_range( new_volume, min_value, max_value );

    (void) output_volume( output_filename, NC_BYTE, FALSE,
                          0.0, 0.0, new_volume, NULL, NULL );


    return( 0 );
}
