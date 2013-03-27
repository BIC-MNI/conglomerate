#include  <bicpl.h>

private  int  tags_exist(
    int             n_objects,
    object_struct   *object_list[],
    int             id1,
    int             id2 );

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
    char                 *landmark_filename, *error_filename;
    VIO_BOOL              left;
    char                 *format;
    VIO_STR               *filenames;
    VIO_Volume               volume;
    volume_input_struct  volume_input;
    int                  n_files, n_objects;
    object_struct        **object_list;
    int                  p;
    int                  n_left, n_right, n_12, n_22;

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

    n_12 = 0;
    n_22 = 0;
    n_left = 0;
    n_right = 0;

    for_less( p, 0, n_files )
    {
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

        if( left )
        {
            if( tags_exist( n_objects, object_list, 12, 13 ) )
                ++n_12;
            ++n_left;
        }
        else
        {
            if( tags_exist( n_objects, object_list, 22, 23 ) )
                ++n_22;
            ++n_right;
        }

        delete_object_list( n_objects, object_list );
    }

    if( volume != (VIO_Volume) NULL )
        cancel_volume_input( volume, &volume_input );

    print( "Branch frequency left : %d out of %d files.\n", n_12, n_left );
    print( "Branch frequency right: %d out of %d files.\n", n_22, n_right );

    return( status != VIO_OK );
}

private  int  tags_exist(
    int             n_objects,
    object_struct   *object_list[],
    int             id1,
    int             id2 )
{
    VIO_BOOL         id1_exists, id2_exists;
    int             i, id;
    marker_struct   *marker;

    id1_exists = FALSE;
    id2_exists = FALSE;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            id = marker->structure_id;
            if( id >= 1000 )
                id -= 1000;

            if( id == id1 )
                id1_exists = TRUE;
            if( id == id2 )
                id2_exists = TRUE;
        }
    }

    return( id1_exists && id2_exists );
}
