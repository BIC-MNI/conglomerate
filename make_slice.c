#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_volume_filename, output_filename;
    STRING               axis_name;
    Volume               volume;
    volume_input_struct  volume_input;
    int                  axis, x_tess, y_tess;
    BOOLEAN              voxel_slice;
    object_struct        *object;
    Real                 world_pos, x_min, x_max, y_min, y_max;
    Real                 world[N_DIMENSIONS], voxel[MAX_DIMENSIONS];
    Real                 xw, yw, zw, xvw, yvw, zvw;
    Point                origin;
    Vector               normal;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  volume_file  output_file  x|y|z  world_pos [xt] [yt]\n",
               argv[0] );
        return( 1 );
    }

    (void) get_string_argument( "", &axis_name );

    if( equal_strings( axis_name, "x" ) )
        axis = X;
    else if( equal_strings( axis_name, "y" ) )
        axis = Y;
    else if( equal_strings( axis_name, "z" ) )
        axis = Z;
    else
        axis = -1;

    if( axis >= 0 )
    {
        voxel_slice = TRUE;

        (void) get_real_argument( 0.0, &world_pos );

        (void) get_int_argument( 0, &x_tess );
        (void) get_int_argument( 0, &y_tess );

        (void) get_real_argument( 0.0, &x_min );
        (void) get_real_argument( 0.0, &x_max );
        (void) get_real_argument( 0.0, &y_min );
        (void) get_real_argument( 0.0, &y_max );
    }
    else
    {
        voxel_slice = FALSE;
        (void) sscanf( axis_name, "%lf", &xw );
        (void) get_real_argument( 0.0, &yw );
        (void) get_real_argument( 0.0, &zw );
        (void) get_real_argument( 0.0, &xvw );
        (void) get_real_argument( 0.0, &yvw );
        (void) get_real_argument( 0.0, &zvw );

        fill_Point( origin, xw, yw, zw );
        fill_Vector( normal, xvw, yvw, zvw );
    }

    if( start_volume_input( input_volume_filename, 3, XYZ_dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != OK )
        return( 1 );

    if( voxel_slice )
    {
        world[X] = 0.0;
        world[Y] = 0.0;
        world[Z] = 0.0;
        world[axis] = world_pos;

        convert_world_to_voxel( volume, world[X], world[Y], world[Z], voxel );

        object = create_object( QUADMESH );
        create_slice_quadmesh( volume, axis, voxel[axis], x_tess, y_tess,
                               x_min, x_max, y_min, y_max,
                               get_quadmesh_ptr(object) );
    }
    else
    {
        object = create_object( POLYGONS );
        create_slice_3d( volume, &origin, &normal, get_polygons_ptr(object) );
    }

    if( output_graphics_file( output_filename, BINARY_FORMAT, 1, &object )
        != OK )
        return( 1 );

    return( 0 );
}
