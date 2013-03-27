#include  <mni.h>

private  void  print_stats(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    VIO_Real            y_min,
    VIO_Real            y_max,
    VIO_BOOL         divide );

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

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Status               status;
    char                 *landmark_filename;
    VIO_STR               *filenames, *left_filenames, *right_filenames;
    VIO_STR               dir, prev_dir;
    VIO_Volume               volume;
    volume_input_struct  volume_input;
    int                  n_files, n_left_objects, n_right_objects;
    object_struct        **left_object_list, **right_object_list;
    VIO_BOOL              divide, left;
    int                  s, p, n_pairs, i, longest, n_printed;

    initialize_argument_processing( argc, argv );

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

    ALLOC( left_filenames, n_files );
    ALLOC( right_filenames, n_files );
    n_pairs = 0;
    prev_dir[0] = (char) 0;

    longest = 0;

    for_less( p, 0, n_files )
    {
        (void) strcpy( dir, filenames[p] );
        s = strlen( dir );
        while( s > 0 && dir[s] != '/' )
            --s;
        dir[s] = (char) 0;

        if( strlen( dir ) > longest )
            longest = strlen( dir );
 
        if( strstr( filenames[p], "cing_l" ) != (char *) NULL )
            left = TRUE;
        else if( strstr( filenames[p], "cing_r" ) != (char *) NULL )
            left = FALSE;
        else
        {
            print( "Filename %s does not match cing_l or cing_r\n",
                   filenames[p] );
            continue;
        }
        if( strcmp( prev_dir, dir ) != 0 )
        {
            left_filenames[n_pairs][0] = (char) 0;
            right_filenames[n_pairs][0] = (char) 0;
            ++n_pairs;
            (void) strcpy( prev_dir, dir );
        }

        if( left )
            (void) strcpy( left_filenames[n_pairs-1], filenames[p] );
        else
            (void) strcpy( right_filenames[n_pairs-1], filenames[p] );
    }

    print( 
" Subject    10   11   12   13   30   32   38    20   21   22   23   40   42   48\n" );
/*
    print( 
"-------- ---- ---- ---- ---- ---- ---- ----  ---- ---- ---- ---- ---- ---- ----\n" );
    print( "\n" );
*/

    for_less( p, 0, n_pairs )
    {
        if( strlen( left_filenames[p] ) == 0 ||
            input_objects_any_format( volume, left_filenames[p],
                            GREEN, 1.0, BOX_MARKER,
                            &n_left_objects, &left_object_list ) == VIO_ERROR )
        {
            n_left_objects = -1;
        }

        if( strlen( right_filenames[p] ) == 0 ||
            input_objects_any_format( volume, right_filenames[p],
                            GREEN, 1.0, BOX_MARKER,
                            &n_right_objects, &right_object_list ) == VIO_ERROR )
        {
            n_right_objects = -1;
        }

        if( strlen( right_filenames[p] ) == 0 )
            (void) strcpy( dir, left_filenames[p] );
        else
            (void) strcpy( dir, right_filenames[p] );

        s = strlen( dir );
        while( s > 0 && dir[s] != '/' )
            --s;
        dir[s] = (char) 0;
        s -= 8;
        if( s < 0 )  s = 0;
 
        for_inclusive( divide, FALSE, FALSE )
        {
            if( !divide )
            {
                print( "%s ", &dir[s] );
                n_printed = strlen( &dir[s] );
            }
            else
                n_printed = 0;

            for_less( i, n_printed, 8 )
                print( " " );

            print_stats( n_left_objects, left_object_list, 10, 0.0, 0.0,
                         divide );
            print_stats( n_left_objects, left_object_list, 11, 0.0, 0.0,
                         divide );
            print_stats( n_left_objects, left_object_list, 12, 0.0, 0.0,
                         divide );
            print_stats( n_left_objects, left_object_list, 13, 0.0, 0.0,
                         divide );
            print_stats( n_left_objects, left_object_list, 30, 0.0, 0.0,
                         divide );
            print_stats( n_left_objects, left_object_list, 30, 12.0, 30.0,
                         divide );
            print_stats( n_left_objects, left_object_list, 30, 12.0, 48.0,
                         divide );

            print( " " );

            print_stats( n_right_objects, right_object_list, 20, 0.0, 0.0,
                         divide );
            print_stats( n_right_objects, right_object_list, 21, 0.0, 0.0,
                         divide );
            print_stats( n_right_objects, right_object_list, 22, 0.0, 0.0,
                         divide );
            print_stats( n_right_objects, right_object_list, 23, 0.0, 0.0,
                         divide );
            print_stats( n_right_objects, right_object_list, 40, 0.0, 0.0,
                         divide );
            print_stats( n_right_objects, right_object_list, 40, 12.0, 30.0,
                         divide );
            print_stats( n_right_objects, right_object_list, 40, 12.0, 48.0,
                         divide );

            print( "\n" );
        }

        delete_object_list( n_left_objects, left_object_list );
        delete_object_list( n_right_objects, right_object_list );
    }

    if( volume != (VIO_Volume) NULL )
        cancel_volume_input( volume, &volume_input );

    return( status != VIO_OK );
}

private  void  print_stats(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    VIO_Real            y_min,
    VIO_Real            y_max,
    VIO_BOOL         divide )
{
    int             i, n_match, n_in_file;
    marker_struct   *marker;

    n_match = 0;
    n_in_file = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            ++n_in_file;
            marker = get_marker_ptr( object_list[i] );

            if( (structure_id < 0 ||
                 marker->structure_id == structure_id ||
                 marker->structure_id == structure_id + 1000) &&
                (y_min >= y_max ||
                 Point_y(marker->position) >= y_min &&
                 Point_y(marker->position) <= y_max ) )
            {
                ++n_match;
            }
        }
    }

    if( n_objects < 0 )
        print( "    M" );
    else if( divide )
        print( "%5.1f", 100.0 * (VIO_Real) n_match / (VIO_Real) n_in_file );
    else
        print( "%5d", n_match );
}
