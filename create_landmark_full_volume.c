#include  <volume_io.h>
#include  <bicpl.h>

#define  SCALE_FACTOR   1.0

static  void  increment_voxel_count(
    VIO_Volume   volume,
    int      x,
    int      y,
    int      z,
    VIO_Real     *max_value )
{
    VIO_Real   value;

    GET_VOXEL_3D_TYPED( value, (VIO_Real), volume, x, y, z );

    value += SCALE_FACTOR;

    if( value > *max_value )
        *max_value = value;

    SET_VOXEL_3D( volume, x, y, z, value );
}

static  VIO_BOOL  is_left_id(
    int   id )
{
    if( id >= 1000 )
        id -= 1000;

    return( id >= 10 && id <= 19 || id == 30 );
}

static  VIO_BOOL  is_right_id(
    int   id )
{
    if( id >= 1000 )
        id -= 1000;

    return( id >= 20 && id <= 29 || id == 40 );
}

static  VIO_BOOL  is_left_filename(
    char   filename[] )
{
    return( strstr( filename, "cing_l" ) != (char *) NULL );
}

extern  int  get_filename_arguments( VIO_STR *[] );

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Status               status;
    char                 *input_filename, *output_filename;
    VIO_Real                 separations[VIO_MAX_DIMENSIONS];
    VIO_Real                 voxel[VIO_MAX_DIMENSIONS];
    VIO_Real                 max_value;
    VIO_Real                 radius, radius_squared;
    VIO_Real                 delta, dx2, dy2, dz2;
    VIO_STR               history;
    VIO_STR               *filenames;
    bitlist_3d_struct    bitlist;
    VIO_Volume               volume, new_volume;
    volume_input_struct  volume_input;
    int                  n_objects, n_files, n_patients, structure_id;
    object_struct        **object_list;
    int                  i, p, x, y, z, n;
    int                  sizes[VIO_N_DIMENSIONS];
    int                  c, min_voxel[VIO_N_DIMENSIONS], max_voxel[VIO_N_DIMENSIONS];
    VIO_Real                 min_limit, max_limit;
    marker_struct        *marker;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_int_argument( 0, &structure_id ) ||
        !get_real_argument( 0.0, &radius ) ||
        !get_int_argument( 0, &n ) )
    {
        print( "%s  example_volume  output_volume  structure_id|-1  n|0 radius of tags  tag_file1 tag_file2 [tag_file3] ...\n", argv[0] );
        return( 1 );
    }

    n_files = get_filename_arguments( &filenames );

    radius_squared = radius * radius;

    status = start_volume_input( input_filename, 3, XYZ_dimension_names,
                                 NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                                 TRUE, &volume, (minc_input_options *) NULL,
                                 &volume_input );

    get_volume_sizes( volume, sizes );
    get_volume_separations( volume, separations );

    /* --- create the output volume */

    new_volume = copy_volume_definition( volume, NC_SHORT, FALSE, 0.0, 0.0 );

    create_bitlist_3d( sizes[VIO_X], sizes[VIO_Y], sizes[VIO_Z], &bitlist );

    for_less( x, 0, sizes[VIO_X] )
        for_less( y, 0, sizes[VIO_Y] )
            for_less( z, 0, sizes[VIO_Z] )
                SET_VOXEL_3D( new_volume, x, y, z, 0.0 );

    max_value = 0.0;

    n_patients = 0;

    for_less( p, 0, n_files )
    {
        if( is_left_id(structure_id) && !is_left_filename(filenames[p]) )
            continue;
        if( is_right_id(structure_id) && is_left_filename(filenames[p]) )
            continue;

        ++n_patients;

        print( "[%d/%d] Reading %s\n", p+1, n_files, filenames[p] );
        (void) flush_file( stdout );

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
                if( structure_id >= 0 &&
                    structure_id != marker->structure_id &&
                    structure_id != marker->structure_id + 1000 )
                    continue;

                convert_world_to_voxel( volume,
                                        (VIO_Real) Point_x(marker->position),
                                        (VIO_Real) Point_y(marker->position),
                                        (VIO_Real) Point_z(marker->position),
                                        voxel );

                if( voxel_is_within_volume( new_volume, voxel ) )
                {
                    if( radius == 0.0 )
                    {
                        x = VIO_ROUND( voxel[VIO_X] );
                        y = VIO_ROUND( voxel[VIO_Y] );
                        z = VIO_ROUND( voxel[VIO_Z] );
                        if( !get_bitlist_bit_3d( &bitlist, x, y, z))
                        {
                            increment_voxel_count( new_volume, x, y, z,
                                                   &max_value );
                            set_bitlist_bit_3d( &bitlist, x, y, z, TRUE );
                        }
                    }
                    else
                    {
                        for_less( c, 0, VIO_N_DIMENSIONS )
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

                        for_inclusive( x, min_voxel[VIO_X], max_voxel[VIO_X] )
                        {
                            delta = ((VIO_Real) x - voxel[VIO_X]) * separations[VIO_X];
                            dx2 = delta * delta;
                            for_inclusive( y, min_voxel[VIO_Y], max_voxel[VIO_Y] )
                            {
                                delta = ((VIO_Real) y - voxel[VIO_Y]) * separations[VIO_Y];
                                dy2 = delta * delta;
                                for_inclusive( z, min_voxel[VIO_Z], max_voxel[VIO_Z] )
                                {
                                    delta = ((VIO_Real) z - voxel[VIO_Z]) *
                                            separations[VIO_Z];
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

    if( structure_id != -1 && structure_id != 1 &&
        !is_left_id(structure_id) && !is_right_id(structure_id) )
        n_patients /= 2;

    if( n > 0 )
        n_patients = n;

    print( "Number subjects: %d\n", n_patients );

    set_volume_voxel_value( new_volume, 0, 0, 0, 0, 0,
                            (VIO_Real) n_patients * SCALE_FACTOR );

    set_volume_voxel_range( new_volume, 0.0, (VIO_Real) n_patients * SCALE_FACTOR );
    set_volume_real_range( new_volume, 0.0, 100.0 );

    cancel_volume_input( volume, &volume_input );

    print( "Writing %s\n", output_filename );

    history = create_string( "Created by:  " );

    for_less( i, 0, argc )
    {
        concat_to_string( &history, " " );
        concat_to_string( &history, argv[i] );
    }

    status = output_volume( output_filename, NC_BYTE, FALSE,
                            0.0, 255.0, new_volume, history,
                            (minc_output_options *) NULL );

    return( status != OK );
}
