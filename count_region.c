#include  <def_mni.h>
#include  <minc.h>

private  void  usage(
    char   executable[] )
{
    print( "Usage:  %s  tag_file  [volume_file_if_lmk] ", executable );
    print( " structure_id [x x_min x_max] [y y_min y_max] [z z_min z_max]\n" );
}

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *tag_filename, *axis_name, *volume_filename;
    int                  n_tags, n_objects, structure_id;
    object_struct        **object_list;
    Real                 min_limit[N_DIMENSIONS], max_limit[N_DIMENSIONS];
    Real                 min_range, max_range;
    int                  i, axis;
    marker_struct        *marker;
    Volume               volume;
    volume_input_struct  volume_input;
    static STRING        in_dim_names[] = {MIxspace, MIyspace, MIzspace};

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &tag_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    status = OK;

    if( filename_extension_matches( tag_filename, 
                                    get_default_landmark_file_suffix() ) )
    {
        if( get_string_argument( "", &volume_filename ) )
        {
            status = start_volume_input( volume_filename, in_dim_names,
                                         FALSE, &volume, &volume_input );
        }
        else
        {
            usage( argv[0] );
            return( 1 );
        }
    }
    else
        volume = (Volume) NULL;

    if( !get_int_argument( 0, &structure_id ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    for_less( axis, 0, N_DIMENSIONS )
    {
        min_limit[axis] = -HUGE;
        max_limit[axis] = HUGE;
    }

    while( get_string_argument( "", &axis_name ) )
    {
        if( strcmp( axis_name, "x" ) == 0 || strcmp( axis_name, "Y" ) == 0 )
            axis = X;
        else if( strcmp( axis_name, "y" ) == 0 || strcmp( axis_name, "Y" ) == 0)
            axis = Y;
        else if( strcmp( axis_name, "z" ) == 0 || strcmp( axis_name, "Z" ) == 0)
            axis = Z;
        else
        {
            usage( argv[0] );
            return( 1 );
        }

        if( !get_real_argument( 0.0, &min_range ) ||
            !get_real_argument( 0.0, &max_range ) )
        {
            usage( argv[0] );
            return( 1 );
        }

        if( min_range < max_range )
        {
            min_limit[axis] = min_range;
            max_limit[axis] = max_range;
        }
    }

    status = input_objects_any_format( volume, tag_filename,
                                       GREEN, 1.0, BOX_MARKER,
                                       &n_objects, &object_list );

    n_tags = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            marker = get_marker_ptr( object_list[i] );
            if( (structure_id == -1 ||
                 marker->structure_id == structure_id ||
                 marker->structure_id == structure_id + 1000) &&
                min_limit[X] <= Point_x(marker->position) &&
                Point_x(marker->position) <= max_limit[X] &&
                min_limit[Y] <= Point_y(marker->position) &&
                Point_y(marker->position) <= max_limit[Y] &&
                min_limit[Z] <= Point_z(marker->position) &&
                Point_z(marker->position) <= max_limit[Z] )
                ++n_tags;
        }
    }

    print( "Tag file    : %s\n", tag_filename );
    print( "Structure id: %d\n", structure_id );

    for_less( axis, 0, N_DIMENSIONS )
    {
        if( min_limit[axis] > -HUGE ||
            max_limit[axis] < HUGE )
        {
            print( "%c Range MM  : %g %g\n", 'X' + axis,
                   min_limit[axis], max_limit[axis] );
        }
    }
    print( "Number      : %d\n", n_tags );
    
    if( volume != (Volume) NULL )
        cancel_volume_input( volume, &volume_input );

    return( status != OK );
}
