#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING   usage_str = "\n\
Usage: %s input.mnc output.mnc x_width y_width z_width\n\
           [world|voxel] [byte]\n\
\n\
     Filters a volume with the given voxel widths, or, if [world] specified,\n\
     then in mm widths.  If byte is specified, then a byte volume is created.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    int              x, y, z, xf, yf, dim;
    int              sizes[N_DIMENSIONS];
    int              filter_start[N_DIMENSIONS];
    int              filter_end[N_DIMENSIONS];
    int              filter_size[N_DIMENSIONS];
    Real             *weights;
    Volume           volume, new_volume;
    STRING           input_filename, output_filename;
    STRING           exec, source_file;
    char             command[EXTREMELY_LARGE_STRING_SIZE];
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

    get_volume_sizes( volume, sizes );

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Zeroing" );

    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    {
        for_less( z, 0, sizes[Z] )
            set_volume_real_value( new_volume, x, y, z, 0, 0, 0.0 );

        update_progress_report( &progress, x * sizes[Y] + y + 1 );
    }

    terminate_progress_report( &progress );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, new_volume, NULL, NULL );

    for_less( dim, 0, N_DIMENSIONS )
    {
        filter_start[dim] = -(sizes[dim]-1);
        filter_end[dim] = sizes[dim]-1;
        filter_size[dim] = filter_end[dim] - filter_start[dim] + 1;
    }

    ALLOC( weights, filter_size[2] );

    initialize_progress_report( &progress, FALSE,
                                filter_size[X] * filter_size[Y], "Filtering" );

    exec = "./tmp.out";
    source_file = "tmp_source.c";

    for_less( xf, 0, filter_size[X] )
    for_less( yf, 0, filter_size[Y] )
    {
        generate_filter( xf, yf, filter_start, filter_size, weights );

        generate_file( source_file, filter_size[Z], weights, sizes );

        (void) sprintf( command, "cc -O3 %s -o %s", source_file, exec );
        system( command );

        unlink( source_file );

        (void) sprintf( command, "%s %s %s", exec, input_filename,
                        output_filename );
        system( command );

        unlink( exec );

        update_progress_report( &progress, yf + xf * filter_size[Y] );
    }

    terminate_progress_report( &progress );

    return( 0 );
}

private  void  generate_filter(
    int           x,
    int           y,
    int           filter_start[],
    int           filter_sizes[],
    Real          weights[] )
{
}
