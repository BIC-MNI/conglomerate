#include  <def_mni.h>
#include  <minc.h>

#define  SCALE_FACTOR   255.0

private  Boolean  get_next_filename(
    char      *filename[] )
{
    static    FILE     *file = 0;
    static    Boolean  in_list = FALSE;
    static    String   filename_string;
    char               *argument;
    Boolean            found;

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

private  void  increment_voxel_count(
    Volume   volume,
    int      x,
    int      y,
    int      z,
    Real     *max_value )
{
    Real   value;

    GET_VOXEL_3D( value, volume, x, y, z );

    value += SCALE_FACTOR;

    if( value > *max_value )
        *max_value = value;

    SET_VOXEL_3D( volume, x, y, z, value );
}

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *input_filename, *landmark_filename, *output_filename;
    Real                 separations[N_DIMENSIONS];
    Real                 voxel[N_DIMENSIONS];
    Real                 max_value;
    Real                 radius, radius_squared;
    Real                 delta, dx2, dy2, dz2;
    char                 history[10000];
    String               *filenames;
    bitlist_3d_struct    bitlist;
    Volume               volume, new_volume;
    volume_input_struct  volume_input;
    int                  n_objects, n_files, n_patients;
    object_struct        **object_list;
    int                  i, p, x, y, z;
    int                  sizes[N_DIMENSIONS];
    int                  c, min_voxel[N_DIMENSIONS], max_voxel[N_DIMENSIONS];
    Real                 min_limit, max_limit;
    marker_struct        *marker;
    static String        in_dim_names[] = { MIxspace, MIyspace, MIzspace };

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &radius ) )
    {
        print( "Need arguments.\n" );
        return( 1 );
    }

    radius_squared = radius * radius;

    status = start_volume_input( input_filename, in_dim_names,
                                 FALSE, &volume, &volume_input );

    get_volume_sizes( volume, sizes );
    get_volume_separations( volume, separations );

    /* --- create the output volume */

    new_volume = create_volume( 3, in_dim_names, NC_SHORT, FALSE );

    set_volume_size( new_volume, NC_UNSPECIFIED, FALSE, sizes );

    set_volume_separations( new_volume, separations );
    copy_general_transform( get_voxel_to_world_transform(volume),
                            get_voxel_to_world_transform(new_volume) );

    alloc_volume_data( new_volume );

    create_bitlist_3d( sizes[X], sizes[Y], sizes[Z], &bitlist );

    for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
            for_less( z, 0, sizes[Z] )
                SET_VOXEL_3D( new_volume, x, y, z, 0.0 );

    n_files = 0;

    while( get_next_filename( &landmark_filename ) )
    {
        SET_ARRAY_SIZE( filenames, n_files, n_files + 1,
                        DEFAULT_CHUNK_SIZE );
        (void) strcpy( filenames[n_files], landmark_filename );
        ++n_files;
    }

    max_value = 0.0;

    for_less( p, 0, n_files )
    {
        print( "[%d/%d] Reading %s\n", p+1, n_files, filenames[p] );

        status = input_objects_any_format( volume, filenames[p],
                                           GREEN, 1.0, BOX_MARKER,
                                           &n_objects, &object_list );

        if( status != OK )
            return( 1 );

        for_less( i, 0, n_objects )
        {
            if( object_list[i]->object_type == MARKER )
            {
                marker = get_marker_ptr( object_list[i] );
                convert_world_to_voxel( volume,
                                        Point_x(marker->position),
                                        Point_y(marker->position),
                                        Point_z(marker->position),
                                        &voxel[X], &voxel[Y], &voxel[Z] );

                if( voxel_is_within_volume( new_volume, voxel ) )
                {
                    if( radius == 0.0 )
                    {
                        increment_voxel_count( new_volume, ROUND(voxel[X]),
                                               ROUND(voxel[Y]), ROUND(voxel[Z]),
                                               &max_value );
                    }
                    else
                    {
                        for_less( c, 0, N_DIMENSIONS )
                        {
                            min_limit = voxel[c] - radius / separations[c];
                            max_limit = voxel[c] + radius / separations[c];
                            min_voxel[c] = CEILING( min_limit );
                            if( min_voxel[c] < 0 )
                                min_voxel[c] = 0;
                            max_voxel[c] = (int) max_limit;
                            if( max_voxel[c] >= sizes[c] )
                                max_voxel[c] = sizes[c]-1;
                        }

                        for_inclusive( x, min_voxel[X], max_voxel[X] )
                        {
                            delta = ((Real) x - voxel[X]) * separations[X];
                            dx2 = delta * delta;
                            for_inclusive( y, min_voxel[Y], max_voxel[Y] )
                            {
                                delta = ((Real) y - voxel[Y]) * separations[Y];
                                dy2 = delta * delta;
                                for_inclusive( z, min_voxel[Z], max_voxel[Z] )
                                {
                                    delta = ((Real) z - voxel[Z]) *
                                            separations[Z];
                                    dz2 = delta * delta;
                                    if( dx2 + dy2 + dz2 <= radius_squared &&
                                        !get_bitlist_bit_3d( &bitlist, x, y, z))
                                    {
                                        increment_voxel_count( new_volume,
                                                               x, y, z,
                                                               &max_value );
                                        set_bitlist_bit_3d( &bitlist, x, y, z,
                                                            TRUE );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        zero_bitlist_3d( &bitlist );
        delete_object_list( n_objects, object_list );
    }

    delete_bitlist_3d( &bitlist );

    n_patients = n_files / 3;

    SET_VOXEL_3D( new_volume, 0, 0, 0, n_patients * SCALE_FACTOR );

    set_volume_voxel_range( new_volume, 0.0, (Real) n_patients * SCALE_FACTOR );
    set_volume_real_range( new_volume, 0.0, 100.0 );

    cancel_volume_input( volume, &volume_input );

    print( "Writing %s\n", output_filename );

    (void) strcpy( history, "Created by:  " );

    for_less( i, 0, argc )
    {
        (void) strcat( history, " " );
        (void) strcat( history, argv[i] );
    }

    status = output_volume( output_filename, TRUE, new_volume, history );

    return( status != OK );
}
