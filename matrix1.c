#include  <mni.h>

#define  COMBINED

private  void  print_stats(
    char            name[],
    int             n_objects,
    object_struct   *object_list[],
    int             min_structure_id,
    int             max_structure_id,
    VIO_Real            y_min_range,
    VIO_Real            y_max_range );

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
    VIO_STR               *filenames, name, prev_name;
    VIO_BOOL              left, left_found, right_found;
    VIO_Volume               volume;
    volume_input_struct  volume_input;
    int                  n_files, n_objects;
    object_struct        **object_list;
    int                  s, p, prev_s;

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

    prev_name[0] = (char) 0;

    for_less( p, 0, n_files )
    {
        status = input_objects_any_format( volume, filenames[p],
                                           GREEN, 1.0, BOX_MARKER,
                                           &n_objects, &object_list );

        if( status != VIO_OK )
            return( 1 );

        if( strstr( filenames[p], "cing_l" ) != (char *) NULL )
            left = TRUE;
        else if( strstr( filenames[p], "cing_r" ) != (char *) NULL )
            left = FALSE;

        (void) strcpy( name, filenames[p] );
        s = strlen( name );
        while( s > 0 && name[s] != '/' )
            --s;
        name[s] = (char) 0;

        s -= 15;
        if( s < 0 )
            s = 0;

        if( strcmp( prev_name, name ) != 0 )
        {
            if( strlen( prev_name ) != 0 )
            {
                if( !left_found )
                {
#ifdef  COMBINED
                    print_stats( &prev_name[prev_s], 0, 0, 10, 13, 1.0, -1.0 );
#else
                    print_stats( &prev_name[prev_s], 0, 0, 10, 10, 1.0, -1.0 );
                    print_stats( &prev_name[prev_s], 0, 0, 11, 11, 1.0, -1.0 );
                    print_stats( &prev_name[prev_s], 0, 0, 12, 12, 1.0, -1.0 );
                    print_stats( &prev_name[prev_s], 0, 0, 13, 13, 1.0, -1.0 );
#endif
                    print_stats( &prev_name[prev_s], 0, 0, 30, 30, 1.0, -1.0 );
                    print_stats( &prev_name[prev_s], 0, 0, 30, 30, 12.0, 30.0 );
                }

                if( !right_found )
                {
#ifdef  COMBINED
                    print_stats( &prev_name[prev_s], 0, 0, 20, 23, 1.0, -1.0 );
#else
                    print_stats( &prev_name[prev_s], 0, 0, 20, 20, 1.0, -1.0 );
                    print_stats( &prev_name[prev_s], 0, 0, 21, 21, 1.0, -1.0 );
                    print_stats( &prev_name[prev_s], 0, 0, 22, 22, 1.0, -1.0 );
                    print_stats( &prev_name[prev_s], 0, 0, 23, 23, 1.0, -1.0 );
#endif
                    print_stats( &prev_name[prev_s], 0, 0, 40, 40, 1.0, -1.0 );
                    print_stats( &prev_name[prev_s], 0, 0, 40, 40, 12.0, 30.0 );
                }
            }
            (void) strcpy( prev_name, name );
            prev_s = s;
            left_found = FALSE;
            right_found = FALSE;
        }

        if( left )
        {
#ifdef  COMBINED
            print_stats( &name[s], n_objects, object_list, 10, 13, 1.0, -1.0 );
#else
            print_stats( &name[s], n_objects, object_list, 10, 10, 1.0, -1.0 );
            print_stats( &name[s], n_objects, object_list, 11, 11, 1.0, -1.0 );
            print_stats( &name[s], n_objects, object_list, 12, 12, 1.0, -1.0 );
            print_stats( &name[s], n_objects, object_list, 13, 13, 1.0, -1.0 );
#endif
            print_stats( &name[s], n_objects, object_list, 30, 30, 1.0, -1.0 );
            print_stats( &name[s], n_objects, object_list, 30, 30, 12.0, 30.0 );
            left_found = TRUE;
        }
        else
        {
#ifdef COMBINED
            print_stats( &name[s], n_objects, object_list, 20, 23, 1.0, -1.0 );
#else
            print_stats( &name[s], n_objects, object_list, 20, 20, 1.0, -1.0 );
            print_stats( &name[s], n_objects, object_list, 21, 21, 1.0, -1.0 );
            print_stats( &name[s], n_objects, object_list, 22, 22, 1.0, -1.0 );
            print_stats( &name[s], n_objects, object_list, 23, 23, 1.0, -1.0 );
#endif
            print_stats( &name[s], n_objects, object_list, 40, 40, 1.0, -1.0 );
            print_stats( &name[s], n_objects, object_list, 40, 40, 12.0, 30.0 );
            right_found = TRUE;
        }

        delete_object_list( n_objects, object_list );
    }

    if( volume != (VIO_Volume) NULL )
        cancel_volume_input( volume, &volume_input );

    return( status != VIO_OK );
}

private  void  print_stats(
    char            name[],
    int             n_objects,
    object_struct   *object_list[],
    int             min_structure_id,
    int             max_structure_id,
    VIO_Real            y_min_range,
    VIO_Real            y_max_range )
{
    int             i, n_samples, id;
    marker_struct   *marker;

    n_samples = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            id = marker->structure_id;
            if( id >= 1000 )  id -= 1000;

            if( (min_structure_id <= id && id <= max_structure_id) &&
                (y_min_range >= y_max_range ||
                 Point_y(marker->position) >= y_min_range &&
                 Point_y(marker->position) <= y_max_range ) )
            {
                ++n_samples;
            }
        }
    }

    if( y_min_range < y_max_range )
    {
        if( min_structure_id == 30 )
            min_structure_id = 32;
        else if( min_structure_id == 40 )
            min_structure_id = 42;
        if( max_structure_id == 30 )
            max_structure_id = 32;
        else if( max_structure_id == 40 )
            max_structure_id = 42;
    }

    if( min_structure_id == max_structure_id )
        print( "%15s %7d %10d\n", name, min_structure_id, n_samples );
    else
        print( "%15s %3d-%3d %10d\n", name, min_structure_id, max_structure_id,
               n_samples );
}
