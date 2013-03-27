#include  <bicpl.h>

private  int  count_occurrences(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id );
private  int  get_n_groups(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    VIO_Real            threshold );

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
     int                  n_files, n_objects, n_in_file, total;
    int                 n_interruptions;
    object_struct        **object_list;
    int                  e, p, n_samples, n_errors;
    int                  structure_id, n_groups;
    VIO_Real                 threshold;

    initialize_argument_processing( argc, argv );

    if( !get_int_argument( 0, &structure_id ) ||
        !get_real_argument( 0.0, &threshold) )
    {
        print( "Usage:  %s  structure_id  threshold  landmark [landmark] ...\n", argv[0] );
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

    n_interruptions = 0;
    total = 0;

    for_less( p, 0, n_files )
    {
        status = input_objects_any_format( volume, filenames[p],
                                           GREEN, 1.0, BOX_MARKER,
                                           &n_objects, &object_list );

        if( strstr( filenames[p], "cing_l" ) != (char *) NULL )
            left = TRUE;
        else if( strstr( filenames[p], "cing_r" ) != (char *) NULL )
            left = FALSE;

        if( status != VIO_OK )
            return( 1 );

        n_in_file = count_occurrences( n_objects, object_list, structure_id );
        if( n_in_file > 0 )
        {
            ++total;
            n_groups = get_n_groups( n_objects, object_list, structure_id,
                                     threshold );

            if( n_groups > 1 )
            {
                ++n_interruptions;
                print( "%50s    Groups: %3d\n", filenames[p], n_groups );
            }
        }

        delete_object_list( n_objects, object_list );
    }

    if( volume != (VIO_Volume) NULL )
        cancel_volume_input( volume, &volume_input );

    print( "\nTotal number of interruptions = %d out of %d\n",
           n_interruptions, total );
    return( status != VIO_OK );
}

private  int  count_occurrences(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id )
{
    int             i, id, n_occurrences;
    marker_struct   *marker;

    n_occurrences = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            id = marker->structure_id;
            if( id >= 1000 )  id -= 1000;

            if( id == structure_id )
                ++n_occurrences;
        }
    }

    return( n_occurrences );
}

private  void   expand_class(
    int    n,
    int    classes[],
    VIO_Point  pos[],
    int    ind,
    int    class,
    VIO_Real   threshold )
{
    int    i;

    classes[ind] = class;

    for_less( i, 0, n )
    {
        if( classes[i] < 0 &&
            distance_between_points( &pos[i], &pos[ind] ) <=  threshold )
            expand_class( n, classes, pos, i, class, threshold );
    }
}

private  int  get_n_groups(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    VIO_Real            threshold )
{
    int             i, id, n, n_groups;
    marker_struct   *marker;
    int             *class;
    VIO_Point           *pos;

    n = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            id = marker->structure_id;
            if( id >= 1000 )  id -= 1000;

            if( id == structure_id )
            {
                SET_ARRAY_SIZE( class, n, n+1, DEFAULT_CHUNK_SIZE );
                SET_ARRAY_SIZE( pos, n, n+1, DEFAULT_CHUNK_SIZE );
                class[n] = -1;
                pos[n] = marker->position;
                ++n;
            }
        }
    }

    n_groups = 0;

    for_less( i, 0, n )
    {
        if( class[i] < 0 )
        {
            expand_class( n, class, pos, i, n_groups, threshold );
            ++n_groups;
        }
    }
    
    FREE( class );
    FREE( pos );


    return( n_groups );
}
