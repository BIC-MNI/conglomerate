#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
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
    Status               status;
    STRING               volume_filename, tag_filename;
    STRING               output_filename, label;
    Real                 voxel[MAX_DIMENSIONS];
    Real                 value_to_set;
    Real                 volume_min, volume_max;
    BOOLEAN              crop_flag;
    STRING               history, dummy;
    Volume               volume, new_volume, used_volume;
    volume_input_struct  volume_input;
    int                  limits[2][MAX_DIMENSIONS];
    int                  structure_id, n_tag_points, n_volumes, tag_id;
    int                  n_used_tag_points;
    int                  n_inside_tag_points;
    Real                 tags1[N_DIMENSIONS];
    int                  i, x, y, z;
    int                  sizes[N_DIMENSIONS];

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
                            &volume_input ) != OK )
        return( 1 );

    /* --- create the output volume */

    new_volume = copy_volume_definition( volume, NC_BYTE, FALSE, 0.0, 255.0 );

    cancel_volume_input( volume, &volume_input );

    get_volume_sizes( new_volume, sizes );
    volume_min = 0.0;
    volume_max = 255.0;
    set_volume_real_range( new_volume, volume_min, volume_max );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                set_volume_real_value( new_volume, x, y, z, 0, 0, 0.0 );
            }
        }
    }

    if( open_file_with_default_suffix( tag_filename,
                                       get_default_tag_file_suffix(),
                                       READ_FILE, ASCII_FORMAT, &file ) != OK ||
        initialize_tag_file_input( file, &n_volumes ) != OK )
    {
        return( 1 );
    }

    n_tag_points = 0;
    n_used_tag_points = 0;
    n_inside_tag_points = 0;

    limits[0][0] = 0;

    while( input_one_tag( file, n_volumes, tags1, NULL, NULL, NULL, &tag_id,
                          &label, NULL ) )
    {
        if( structure_id < 0 || structure_id == tag_id )
        {
            convert_world_to_voxel( new_volume,
                                    tags1[X], tags1[Y], tags1[Z], voxel );

            if( voxel_is_within_volume( new_volume, voxel ) )
            {
                x = ROUND( voxel[X] );
                y = ROUND( voxel[Y] );
                z = ROUND( voxel[Z] );

                value_to_set = (Real) tag_id;
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
                    limits[0][X] = x;
                    limits[0][Y] = y;
                    limits[0][Z] = z;
                    limits[1][X] = x;
                    limits[1][Y] = y;
                    limits[1][Z] = z;
                }
                else
                {
                    if( x < limits[0][X] )
                        limits[0][X] = x;
                    else if( x > limits[1][X] )
                        limits[1][X] = x;

                    if( y < limits[0][Y] )
                        limits[0][Y] = y;
                    else if( y > limits[1][Y] )
                        limits[1][Y] = y;

                    if( z < limits[0][Z] )
                        limits[0][Z] = z;
                    else if( z > limits[1][Z] )
                        limits[1][Z] = z;
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

    return( status != OK );
}
