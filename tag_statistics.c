#include  <mni.h>

private  BOOLEAN  get_stats_for_one_file(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    Real            *x_min,
    Real            *x_max,
    Real            *x_mean,
    Real            *y_min,
    Real            *y_max,
    Real            *y_mean,
    Real            *z_min,
    Real            *z_max,
    Real            *z_mean );

private  BOOLEAN  get_next_filename(
    char      *filename[] )
{
    static    FILE     *file = 0;
    static    BOOLEAN  in_list = FALSE;
    static    STRING   filename_string;
    char               *argument;
    BOOLEAN            found;

    found = FALSE;

    do
    {
        if( in_list )
        {
            if( input_string( file, filename_string, MAX_STRING_LENGTH, ' ' )
                 == OK )
            {
                *filename = filename_string;
                found = TRUE;
            }
            else
            {
                (void) close_file( file );
                in_list = FALSE;
            }
        }
        else if( get_string_argument( "", &argument ) )
        {
            if( filename_extension_matches( argument,
                                get_default_landmark_file_suffix() )||
                filename_extension_matches( argument,
                                get_default_tag_file_suffix() ) ||
                filename_extension_matches( argument, "obj" ) )
            {
                *filename = argument;
                found = TRUE;
            }
            else
            {
                if( open_file( argument, READ_FILE, ASCII_FORMAT, &file ) != OK)
                    break;
                in_list = TRUE;
            }
        }
        else
            break;
    }
    while( !found );

    return( found );
}

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *landmark_filename;
    Real                 *x_mins, *x_maxs, *y_mins, *y_maxs, *z_mins, *z_maxs;
    Real                 *x_means, *y_means, *z_means;
    Real                 x_min, x_max, y_min, y_max, z_min, z_max;
    Real                 x_mean, y_mean, z_mean;
    Real                 min_value, max_value, mean, std_dev, median;
    STRING               *filenames;
    Volume               volume;
    volume_input_struct  volume_input;
    int                  n_files, n_objects;
    object_struct        **object_list;
    int                  p, n_samples;
    int                  structure_id;

    initialize_argument_processing( argc, argv );

    if( !get_int_argument( 0, &structure_id ) )
    {
        print( "Usage:  %s  structure_id  landmark [landmark] ...\n", argv[0] );
        return( 1 );
    }

    n_files = 0;
    volume = (Volume) NULL;

    while( get_next_filename( &landmark_filename ) )
    {
        if( filename_extension_matches( landmark_filename, "mnc" ) ||
            filename_extension_matches( landmark_filename, "mni" ) ||
            filename_extension_matches( landmark_filename, "fre" ) )
        {
            if( volume != (Volume) NULL )
                cancel_volume_input( volume, &volume_input );
 
            status = start_volume_input(
                          landmark_filename, 3, XYZ_dimension_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          TRUE, &volume, (minc_input_options *) NULL,
                          &volume_input );
        }
        else
        {
            SET_ARRAY_SIZE( filenames, n_files, n_files + 1,
                            DEFAULT_CHUNK_SIZE );
            (void) strcpy( filenames[n_files], landmark_filename );
            ++n_files;
        }
    }

    n_samples = 0;

    for_less( p, 0, n_files )
    {
        print( "[%d/%d] Reading %s\n", p+1, n_files, filenames[p] );

        status = input_objects_any_format( volume, filenames[p],
                                           GREEN, 1.0, BOX_MARKER,
                                           &n_objects, &object_list );

        if( status != OK )
            return( 1 );

        if( get_stats_for_one_file( n_objects, object_list, structure_id,
                                    &x_min, &x_max, &x_mean,
                                    &y_min, &y_max, &y_mean,
                                    &z_min, &z_max, &z_mean ) )
        {
            SET_ARRAY_SIZE( x_mins, n_samples, n_samples+1, DEFAULT_CHUNK_SIZE);
            SET_ARRAY_SIZE( x_maxs, n_samples, n_samples+1, DEFAULT_CHUNK_SIZE);
            SET_ARRAY_SIZE( x_means, n_samples, n_samples+1,DEFAULT_CHUNK_SIZE);
            SET_ARRAY_SIZE( y_mins, n_samples, n_samples+1, DEFAULT_CHUNK_SIZE);
            SET_ARRAY_SIZE( y_maxs, n_samples, n_samples+1, DEFAULT_CHUNK_SIZE);
            SET_ARRAY_SIZE( y_means, n_samples, n_samples+1,DEFAULT_CHUNK_SIZE);
            SET_ARRAY_SIZE( z_mins, n_samples, n_samples+1, DEFAULT_CHUNK_SIZE);
            SET_ARRAY_SIZE( z_maxs, n_samples, n_samples+1, DEFAULT_CHUNK_SIZE);
            SET_ARRAY_SIZE( z_means, n_samples, n_samples+1,DEFAULT_CHUNK_SIZE);

            x_mins[n_samples] = x_min;
            x_maxs[n_samples] = x_max;
            x_means[n_samples] = x_mean;
            y_mins[n_samples] = y_min;
            y_maxs[n_samples] = y_max;
            y_means[n_samples] = y_mean;
            z_mins[n_samples] = z_min;
            z_maxs[n_samples] = z_max;
            z_means[n_samples] = z_mean;

            ++n_samples;
        }
    }

    if( volume != (Volume) NULL )
        cancel_volume_input( volume, &volume_input );

    if( n_samples == 0 )
        return( 0 );

    if( structure_id < 0 )
        print( "Statistics for ALL structure ids together\n" );
    else
        print( "Statistics for structure id: %d\n", structure_id );

    print( "-------------------------------\n" );

    compute_statistics( n_samples, x_means, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "X means: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, y_means, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "Y means: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, z_means, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "Z means: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    print( "\n" );

    compute_statistics( n_samples, x_mins, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "X minimum: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, x_maxs, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "X maximum: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, y_mins, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "Y minimum: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, y_maxs, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "Y maximum: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, z_mins, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "Z minimum: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, z_maxs, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( "Z maximum: [ %g , %g ]  mean: %g   dev: %g  median %g\n",
           min_value, max_value, mean, std_dev, median );

    return( status != OK );
}

private  BOOLEAN  get_stats_for_one_file(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    Real            *x_min,
    Real            *x_max,
    Real            *x_mean,
    Real            *y_min,
    Real            *y_max,
    Real            *y_mean,
    Real            *z_min,
    Real            *z_max,
    Real            *z_mean )
{
    int             i, n_samples;
    Real            *x_positions, *y_positions, *z_positions;
    Real            std_dev, median;
    marker_struct   *marker;

    n_samples = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );

            if( structure_id < 0 || marker->structure_id == structure_id )
            {
                SET_ARRAY_SIZE( x_positions, n_samples, n_samples+1,
                                DEFAULT_CHUNK_SIZE );
                SET_ARRAY_SIZE( y_positions, n_samples, n_samples+1,
                                DEFAULT_CHUNK_SIZE );
                SET_ARRAY_SIZE( z_positions, n_samples, n_samples+1,
                                DEFAULT_CHUNK_SIZE );

                x_positions[n_samples] = Point_x(marker->position);
                y_positions[n_samples] = Point_y(marker->position);
                z_positions[n_samples] = Point_z(marker->position);

                ++n_samples;
            }
        }
    }

    if( n_samples > 0 )
    {
        compute_statistics( n_samples, x_positions, x_min, x_max,
                            x_mean, &std_dev, &median );
        compute_statistics( n_samples, y_positions, y_min, y_max,
                            y_mean, &std_dev, &median );
        compute_statistics( n_samples, z_positions, z_min, z_max,
                            z_mean, &std_dev, &median );
    }

    return( n_samples > 0 );
}
