#include  <volume_io.h>
#include  <bicpl.h>

private  int  get_stats_for_one_file(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    VIO_Real            y_min_range,
    VIO_Real            y_max_range,
    VIO_Real            *x_min,
    VIO_Real            *x_max,
    VIO_Real            *x_mean,
    VIO_Real            *y_min,
    VIO_Real            *y_max,
    VIO_Real            *y_mean,
    VIO_Real            *z_min,
    VIO_Real            *z_max,
    VIO_Real            *z_mean,
    int             *total_in_file );

private  VIO_BOOL  get_next_filename(
    char      *filename[] )
{
    static    FILE     *file = 0;
    static    VIO_BOOL  in_list = FALSE;
    static    VIO_STR   filename_string;
    char               *argument;
    VIO_BOOL            found;

    found = FALSE;

    do
    {
        if( in_list )
        {
            if( input_string( file, filename_string, MAX_STRING_LENGTH, ' ' )
                 == VIO_OK )
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
                if( open_file( argument, READ_FILE, ASCII_FORMAT, &file ) != VIO_OK)
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

private  VIO_BOOL  is_left_id(
    int   id )
{
    if( id >= 1000 )
        id -= 1000;

    return( id >= 10 && id <= 19 || id == 30 );
}

private  VIO_BOOL  is_right_id(
    int   id )
{
    if( id >= 1000 )
        id -= 1000;

    return( id >= 20 && id <= 29 || id == 40 );
}

#define  MAX_IDS  1100

private  int   find_left_right_error(
    VIO_BOOL         left,
    int             n_objects,
    object_struct   *object_list[],
    int             id_errors[] )
{
    int            i, n_errors, id;
    marker_struct  *marker;

    n_errors = 0;

    for_less( i, 0, MAX_IDS )
        id_errors[i] = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            id = marker->structure_id;
            if( id >= 1000 ) id -= 1000;

            if( left && !is_left_id( id ) || !left && !is_right_id( id ) )
            {
                ++id_errors[id];
                ++n_errors;
            }
        }
    }

    if( id_errors[1] < n_objects )
    {
        n_errors -= id_errors[1];
        id_errors[1] = 0;
    }

    return( n_errors );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Status               status;
    char                 *landmark_filename, *error_filename;
    VIO_Real                 *x_mins, *x_maxs, *y_mins, *y_maxs, *z_mins, *z_maxs;
    VIO_Real                 *x_means, *y_means, *z_means;
    VIO_Real                 x_min, x_max, y_min, y_max, z_min, z_max;
    VIO_Real                 x_mean, y_mean, z_mean;
    VIO_Real                 min_value, max_value, mean, std_dev, median;
    VIO_Real                 y_min_range, y_max_range;
    VIO_BOOL              left;
    FILE                 *left_right_errors;
    char                 *format;
    VIO_STR               *filenames;
    int                  id_errors[MAX_IDS];
    VIO_Volume               volume;
    volume_input_struct  volume_input;
    int                  n_files, n_objects, n_files_for_patient, total_in_file;
    object_struct        **object_list;
    int                  e, p, n_samples, n_errors;
    int                  structure_id, n_in_file;

    initialize_argument_processing( argc, argv );

    if( !get_int_argument( 0, &structure_id ) ||
        !get_real_argument( 0.0, &y_min_range ) ||
        !get_real_argument( 0.0, &y_max_range ) ||
        !get_string_argument( "", &error_filename ) )
    {
        print( "Usage:  %s  structure_id  y_min y_max error_file landmark [landmark] ...\n", argv[0] );
        return( 1 );
    }

    n_files = 0;
    volume = (VIO_Volume) NULL;

    while( get_next_filename( &landmark_filename ) )
    {
        if( filename_extension_matches( landmark_filename, "mnc" ) ||
            filename_extension_matches( landmark_filename, "mni" ) ||
            filename_extension_matches( landmark_filename, "fre" ) )
        {
            if( volume != (VIO_Volume) NULL )
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

    status = open_file( error_filename, WRITE_FILE, ASCII_FORMAT,
                        &left_right_errors );

    for_less( p, 0, n_files )
    {
/*
        print( "[%d/%d] Reading %s\n", p+1, n_files, filenames[p] );
*/
        status = input_objects_any_format( volume, filenames[p],
                                           GREEN, 1.0, BOX_MARKER,
                                           &n_objects, &object_list );

        if( strstr( filenames[p], "cing_l" ) != (char *) NULL )
            left = TRUE;
        else if( strstr( filenames[p], "cing_r" ) != (char *) NULL )
            left = FALSE;
        else
        {
            print( "Filename %s does not match cing_l or cing_r\n",
                   filenames[p] );
            break;
        }

        n_errors = find_left_right_error( left, n_objects, object_list,
                                          id_errors );

        if( n_errors > 0 )
        {
            (void) fprintf( left_right_errors, "File %s: \t%d wrong ids: \t",
                            filenames[p], n_errors );

            for_less( e, 0, MAX_IDS )
                if( id_errors[e] > 0 )
                    (void) fprintf( left_right_errors, " %d(%d)", e,
                                    id_errors[e] );
            (void) fprintf( left_right_errors, "\n" );
        }

        if( status != VIO_OK )
            return( 1 );


        if( structure_id < 0 ||
            left && is_left_id(structure_id) ||
            !left && is_right_id(structure_id) )
        {
            n_in_file = get_stats_for_one_file( n_objects, object_list,
                                        structure_id,
                                        y_min_range, y_max_range,
                                        &x_min, &x_max, &x_mean,
                                        &y_min, &y_max, &y_mean,
                                        &z_min, &z_max, &z_mean,
                                        &total_in_file );
            if( n_in_file > 0 )
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

        delete_object_list( n_objects, object_list );
    }

    status = close_file( left_right_errors );

    if( volume != (VIO_Volume) NULL )
        cancel_volume_input( volume, &volume_input );

    if( n_samples == 0 )
        return( 0 );

    print( "\n" );
    print( "----------------------------------------------------------------------------\n" );

    if( structure_id < 0 )
        print( "           Statistics for ALL structure ids together (%d samples)\n", n_samples );
    else
        print( "           Statistics for structure id: %d (%d samples)\n", structure_id, n_samples );

    if( y_min_range < y_max_range )
        print( "           for only those tags between %g mm and %g mm in the Y direction.\n", y_min_range, y_max_range );

    print( "----------------------------------------------------------------------------\n" );
    print( "\n" );

    print( "  Variable       Minimum      Maximum       Mean       Std. Dev.      Median\n" );
    print( "  --------      --------     --------     --------    ----------     -------\n" );
    print( "\n" );

    format = "%10s: %12.2f %12.2f %12.2f %12.2f %12.2f\n";

    compute_statistics( n_samples, x_means, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "X mean", min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, y_means, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "Y mean", min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, z_means, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "Z mean", min_value, max_value, mean, std_dev, median );

    print( "\n" );

    compute_statistics( n_samples, x_mins, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "X minimum", min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, x_maxs, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "X maximum", min_value, max_value, mean, std_dev, median );

    print( "\n" );

    compute_statistics( n_samples, y_mins, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "Y minimum", min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, y_maxs, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "Y maximum", min_value, max_value, mean, std_dev, median );

    print( "\n" );

    compute_statistics( n_samples, z_mins, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "Z minimum", min_value, max_value, mean, std_dev, median );

    compute_statistics( n_samples, z_maxs, &min_value, &max_value,
                        &mean, &std_dev, &median );
    print( format, "Z maximum", min_value, max_value, mean, std_dev, median );

    print( "\n" );

    return( status != VIO_OK );
}

private  int  get_stats_for_one_file(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    VIO_Real            y_min_range,
    VIO_Real            y_max_range,
    VIO_Real            *x_min,
    VIO_Real            *x_max,
    VIO_Real            *x_mean,
    VIO_Real            *y_min,
    VIO_Real            *y_max,
    VIO_Real            *y_mean,
    VIO_Real            *z_min,
    VIO_Real            *z_max,
    VIO_Real            *z_mean,
    int             *total_in_file )
{
    int             i, n_samples;
    VIO_Real            *x_positions, *y_positions, *z_positions;
    VIO_Real            std_dev, median;
    marker_struct   *marker;

    *total_in_file = 0;
    n_samples = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            ++(*total_in_file);
            marker = get_marker_ptr( object_list[i] );

            if( (structure_id < 0 ||
                 marker->structure_id == structure_id ||
                 marker->structure_id == structure_id + 1000) &&
                (y_min_range >= y_max_range ||
                 Point_y(marker->position) >= y_min_range &&
                 Point_y(marker->position) <= y_max_range ) )
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

        FREE( x_positions );
        FREE( y_positions );
        FREE( z_positions );
    }

    return( n_samples );
}
