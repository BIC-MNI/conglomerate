#include  <module.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_volume_filename, *output_filename;
    char                 *axis_name;
    Volume               volume;
    volume_input_struct  volume_input;
    int                  axis;
    object_struct        *object;
    Real                 world_pos;
    Real                 world[N_DIMENSIONS], voxel[MAX_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_string_argument( "", &axis_name ) ||
        !get_real_argument( 0.0, &world_pos ) )
    {
        print( "Usage: %s  volume_file  output_file  x|y|z  world_pos\n",
               argv[0] );
        return( 1 );
    }

    if( strcmp( axis_name, "x" ) == 0 )
        axis = X;
    else if( strcmp( axis_name, "y" ) == 0 )
        axis = Y;
    else if( strcmp( axis_name, "z" ) == 0 )
        axis = Z;
    else
        axis = X;

    if( start_volume_input( input_volume_filename, 3, XYZ_dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != OK )
        return( 1 );

    world[X] = 0.0;
    world[Y] = 0.0;
    world[Z] = 0.0;
    world[axis] = world_pos;

    convert_world_to_voxel( volume, world[X], world[Y], world[Z], voxel );

    object = create_object( QUADMESH );
    create_slice_quadmesh( volume, axis, voxel[axis],
                           get_quadmesh_ptr(object) );

    if( output_graphics_file( output_filename, BINARY_FORMAT, 1, &object )
        != OK )
        return( 1 );

    return( 0 );
}
