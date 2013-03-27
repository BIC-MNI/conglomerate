#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s input.mnc output.mnc  [max_dilations|-1] [6|26] [min_outside max_outside]\n\
                [min_inside max_inside]\n\
     Dilates all regions of volume until there are no values in the range\n\
     (min_outside, max_outside), which defaults to 0 0\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               input_filename, output_filename;
    VIO_Volume               volume;
    int                  n_neighs, iter, max_dilations;
    int                  sizes[VIO_N_DIMENSIONS];
    int                  x, y, z, n_changed;
    int                  tx, ty, tz, n_dirs, *dx, *dy, *dz, dir;
    VIO_BOOL              found;
    VIO_Real                 max_value, test_value, value;
    VIO_Real                 min_outside, max_outside;
    VIO_Real                 min_inside, max_inside;
    VIO_Real                 **output_buffer[2], **tmp;
    Neighbour_types      connectivity;
    VIO_progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -1, &max_dilations );
    (void) get_int_argument( 26, &n_neighs );
    (void) get_real_argument( 0.0, &min_outside );
    (void) get_real_argument( 0.0, &max_outside );
    (void) get_real_argument( 0.0, &min_inside );
    (void) get_real_argument( -1.0, &max_inside );

    switch( n_neighs )
    {
    case 6:   connectivity = FOUR_NEIGHBOURS;  break;
    case 26:   connectivity = EIGHT_NEIGHBOURS;  break;
    default:  print_error( "# neighs must be 6 or 26.\n" );  return( 1 );
    }

    if( input_volume( input_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    n_dirs = get_3D_neighbour_directions( connectivity, &dx, &dy, &dz );

    iter = 0;

    get_volume_sizes( volume, sizes );

    VIO_ALLOC2D( output_buffer[0], sizes[VIO_Y], sizes[VIO_Z] );
    VIO_ALLOC2D( output_buffer[1], sizes[VIO_Y], sizes[VIO_Z] );

    n_changed = 1;
    while( (max_dilations < 0 || iter < max_dilations) && n_changed > 0 )
    {
        ++iter;

        n_changed = 0;

        initialize_progress_report( &progress, FALSE, sizes[VIO_X], "Dilating" );

        for_less( x, 0, sizes[VIO_X] )
        {
            for_less( y, 0, sizes[VIO_Y] )
            {
                for_less( z, 0, sizes[VIO_Z] )
                {
                    value = get_volume_real_value( volume, x, y, z, 0, 0 );
                    found = FALSE;
                    if( min_outside <= value && value <= max_outside )
                    {
                        max_value = 0.0;
                        for_less( dir, 0, n_dirs )
                        {
                            tx = x + dx[dir];
                            ty = y + dy[dir];
                            tz = z + dz[dir];
                            if( tx >= 0 && tx < sizes[0] &&
                                ty >= 0 && ty < sizes[1] &&
                                tz >= 0 && tz < sizes[2] )
                            {
                                test_value = get_volume_real_value( volume,
                                                            tx, ty, tz, 0, 0 );

                                if( min_inside > max_inside ||
                                    min_inside <= test_value &&
                                    test_value <= max_inside )
                                {
                                    if( !found )
                                    {
                                        max_value = test_value;
                                        found = TRUE;
                                    }
                                    else
                                        max_value = MAX(max_value,test_value);
                                }
                            }
                        }

                    }

                    if( !found )
                        max_value = value;

                    output_buffer[1][y][z] = max_value;

                    if( max_value != value )
                        ++n_changed;
                }
            }

            if( x > 0 )
            {
                for_less( y, 0, sizes[VIO_Y] )
                for_less( z, 0, sizes[VIO_Z] )
                {
                    set_volume_real_value( volume, x-1, y, z, 0, 0,
                                           output_buffer[0][y][z] );
                }
            }

            if( x == sizes[VIO_X]-1 )
            {
                for_less( y, 0, sizes[VIO_Y] )
                for_less( z, 0, sizes[VIO_Z] )
                {
                    set_volume_real_value( volume, x, y, z, 0, 0,
                                           output_buffer[1][y][z] );
                }
            }

            tmp = output_buffer[0];
            output_buffer[0] = output_buffer[1];
            output_buffer[1] = tmp;

            update_progress_report( &progress, x + 1 );
        }

        terminate_progress_report( &progress );

        print( "Iter: %d    N changed: %d\n", iter, n_changed );
    }

    VIO_FREE2D( output_buffer[0] );
    VIO_FREE2D( output_buffer[1] );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, volume, input_filename,
                                   "dilate_volume_completely\n",
                                   NULL );

    return( 0 );
}
