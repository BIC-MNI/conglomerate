#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  SCALE_FACTOR   1.0

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

private  BOOLEAN  is_left_id(
    int   id )
{
    if( id >= 1000 )
        id -= 1000;

    return( id >= 10 && id <= 19 || id == 30 );
}

private  BOOLEAN  is_right_id(
    int   id )
{
    if( id >= 1000 )
        id -= 1000;

    return( id >= 20 && id <= 29 || id == 40 );
}

private  BOOLEAN  is_left_filename(
    char   filename[] )
{
    return( strstr( filename, "cing_l" ) != (char *) NULL );
}

extern  int  get_filename_arguments( STRING *[] );

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *input_filename, *output_filename;
    Real                 separations[MAX_DIMENSIONS];
    Real                 voxel[MAX_DIMENSIONS];
    Real                 max_value;
    Real                 radius, radius_squared;
    Real                 delta, dx2, dy2, dz2;
    char                 history[10000];
    STRING               *filenames;
    bitlist_3d_struct    bitlist;
    Volume               volume, new_volume;
    volume_input_struct  volume_input;
    int                  n_objects, n_files, n_patients, structure_id;
    object_struct        **object_list;
    int                  i, p, x, y, z, n;
    int                  sizes[N_DIMENSIONS];
    int                  c, min_voxel[N_DIMENSIONS], max_voxel[N_DIMENSIONS];
    Real                 min_limit, max_limit;
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

    create_bitlist_3d( sizes[X], sizes[Y], sizes[Z], &bitlist );

    for_less( x, 0, sizes[X] )
        for_less( y, 0, sizes[Y] )
            for_less( z, 0, sizes[Z] )
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
                                        Point_x(marker->position),
                                        Point_y(marker->position),
                                        Point_z(marker->position),
                                        voxel );

                if( voxel_is_within_volume( new_volume, voxel ) )
                {
                    if( radius == 0.0 )
                    {
                        x = ROUND( voxel[X] );
                        y = ROUND( voxel[Y] );
                        z = ROUND( voxel[Z] );
                        if( !get_bitlist_bit_3d( &bitlist, x, y, z))
                        {
                            increment_voxel_count( new_volume, x, y, z,
                                                   &max_value );
                            set_bitlist_bit_3d( &bitlist, x, y, z, TRUE );
                        }
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

    if( structure_id != -1 && structure_id != 1 &&
        !is_left_id(structure_id) && !is_right_id(structure_id) )
        n_patients /= 2;

    if( n > 0 && n >= n_patients )
        n_patients = n;

    print( "Number subjects: %d\n", n_patients );

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

    status = output_volume( output_filename, NC_BYTE, FALSE,
                            0.0, 255.0, new_volume, history,
                            (minc_output_options *) NULL );

    return( status != OK );
}
