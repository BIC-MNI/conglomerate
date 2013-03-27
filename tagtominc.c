#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: tagtominc  example_volume.mnc  input.tag  output.mnc  [structure_id|-1] [crop]\n\
\n\
     Converts a tag file to a MINC volume with values equal the tags'\n\
     structure ids, given an example MINC volume.\n\
     If structure_id is specified, then only tags with this id are used.\n\
     Otherwise all tags are used.  If crop is specified, then the minc\n\
     file is cropped to the smallest possible size.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    FILE                 *file;
    VIO_Status               status;
    VIO_STR               volume_filename, tag_filename;
    VIO_STR               output_filename, label;
    VIO_Real                 voxel[VIO_MAX_DIMENSIONS];
    VIO_Real                 value_to_set;
    VIO_Real                 volume_min, volume_max;
    VIO_BOOL              crop_flag;
    VIO_STR               history, dummy;
    VIO_Volume               volume, new_volume, used_volume;
    volume_input_struct  volume_input;
    int                  limits[2][VIO_MAX_DIMENSIONS];
    int                  structure_id, n_tag_points, n_volumes, tag_id;
    int                  n_used_tag_points;
    int                  n_inside_tag_points;
    VIO_Real                 tags1[VIO_N_DIMENSIONS];
    int                  i, x, y, z;
    int                  sizes[VIO_N_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &tag_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( -1, &structure_id );
    crop_flag = get_string_argument( NULL, &dummy );

    if( start_volume_input( volume_filename, 3, XYZ_dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != VIO_OK )
        return( 1 );

    /* --- create the output volume */

    new_volume = copy_volume_definition( volume, NC_BYTE, FALSE, 0.0, 255.0 );

    cancel_volume_input( volume, &volume_input );

    get_volume_sizes( new_volume, sizes );
    volume_min = 0.0;
    volume_max = 255.0;
    set_volume_real_range( new_volume, volume_min, volume_max );

    for_less( x, 0, sizes[VIO_X] )
    {
        for_less( y, 0, sizes[VIO_Y] )
        {
            for_less( z, 0, sizes[VIO_Z] )
            {
                set_volume_real_value( new_volume, x, y, z, 0, 0, 0.0 );
            }
        }
    }

    if( open_file_with_default_suffix( tag_filename,
                                       get_default_tag_file_suffix(),
                                       READ_FILE, ASCII_FORMAT, &file ) != VIO_OK ||
        initialize_tag_file_input( file, &n_volumes ) != VIO_OK )
    {
        return( 1 );
    }

    n_tag_points = 0;
    n_used_tag_points = 0;
    n_inside_tag_points = 0;

    limits[0][0] = 0;

    while( input_one_tag( file, n_volumes, tags1, NULL, NULL, &tag_id, NULL,
                          &label, NULL ) )
    {
        if( structure_id < 0 || structure_id == tag_id )
        {
            convert_world_to_voxel( new_volume,
                                    tags1[VIO_X], tags1[VIO_Y], tags1[VIO_Z], voxel );

            if( voxel_is_within_volume( new_volume, voxel ) )
            {
                x = VIO_ROUND( voxel[VIO_X] );
                y = VIO_ROUND( voxel[VIO_Y] );
                z = VIO_ROUND( voxel[VIO_Z] );

                value_to_set = (VIO_Real) tag_id;
                if( value_to_set < volume_min || value_to_set > volume_max )
                {
                    if( label == NULL ||
                        sscanf( label, "%lf", &value_to_set ) != 1 ||
                        value_to_set < volume_min || value_to_set > volume_max )
                    value_to_set = volume_max;
                }

                set_volume_real_value( new_volume, x, y, z, 0, 0,
                                       value_to_set );

                if( n_inside_tag_points == 0 )
                {
                    limits[0][VIO_X] = x;
                    limits[0][VIO_Y] = y;
                    limits[0][VIO_Z] = z;
                    limits[1][VIO_X] = x;
                    limits[1][VIO_Y] = y;
                    limits[1][VIO_Z] = z;
                }
                else
                {
                    if( x < limits[0][VIO_X] )
                        limits[0][VIO_X] = x;
                    else if( x > limits[1][VIO_X] )
                        limits[1][VIO_X] = x;

                    if( y < limits[0][VIO_Y] )
                        limits[0][VIO_Y] = y;
                    else if( y > limits[1][VIO_Y] )
                        limits[1][VIO_Y] = y;

                    if( z < limits[0][VIO_Z] )
                        limits[0][VIO_Z] = z;
                    else if( z > limits[1][VIO_Z] )
                        limits[1][VIO_Z] = z;
                }

                ++n_inside_tag_points;
            }

            ++n_used_tag_points;
        }
        ++n_tag_points;
    }

    if( n_tag_points == 0 )
        print( "File %s contains no tag points.\n", tag_filename );
    else if( n_inside_tag_points == n_tag_points )
        print( "Converted all %d tags to volume.\n", n_tag_points );
    else
    {
        print( "Converted %d out of %d tags to volume.\n", n_used_tag_points,
               n_tag_points );
        if( n_inside_tag_points != n_used_tag_points )
        {
            print( "          discarded %d of these because they were outside the range of the volume.\n",
                   n_used_tag_points - n_inside_tag_points, n_used_tag_points );
        }
    }

    history = create_string( "Created by:  " );

    for_less( i, 0, argc )
    {
        concat_to_string( &history, " " );
        concat_to_string( &history, argv[i] );
    }
    concat_to_string( &history, "\n" );

    used_volume = new_volume;

    if( crop_flag )
        used_volume = create_cropped_volume( new_volume, limits );

    status = output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                            0.0, 0.0, used_volume, volume_filename, history,
                            (minc_output_options *) NULL );

    return( status != VIO_OK );
}
