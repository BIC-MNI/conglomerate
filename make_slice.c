#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_volume_filename, output_filename;
    STRING               axis_name, space;
    Volume               volume;
    volume_input_struct  volume_input;
    int                  axis, x_tess, y_tess, a1, a2;
    BOOLEAN              voxel_slice, volume_present;
    object_struct        *object;
    polygons_struct      *polygons;
    Real                 pos, x_min, x_max, y_min, y_max;
    Real                 voxel[MAX_DIMENSIONS];
    Real                 xw, yw, zw, xvw, yvw, zvw;
    Point                origin;
    Vector               normal;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print( "Usage: %s  volume_file  output_file  x|y|z  v|w pos [xt] [yt]\n",
               argv[0] );
        print( "or Usage: %s  -  output_file  x|y|z  pos [min1 max1 min2 max2\n", argv[0] );
        return( 1 );
    }

    (void) get_string_argument( NULL, &axis_name );

    volume_present = !equal_strings( input_volume_filename, "-" );

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
        if( volume_present )
            (void) get_string_argument( NULL, &space );

        voxel_slice = TRUE;

        (void) get_real_argument( 0.0, &pos );

        if( volume_present )
        {
            (void) get_int_argument( 0, &x_tess );
            (void) get_int_argument( 0, &y_tess );
        }

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

    if( volume_present &&
        start_volume_input( input_volume_filename, 3, XYZ_dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != OK )
        return( 1 );

    if( volume_present )
    {
        if( voxel_slice )
        {
            voxel[X] = 0.0;
            voxel[Y] = 0.0;
            voxel[Z] = 0.0;
            voxel[axis] = pos;

            if( space[0] == 'w' || space[0] == 'W' )
                convert_world_to_voxel( volume, voxel[X], voxel[Y], voxel[Z],
                                        voxel );

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
    }
    else
    {
        object = create_object( POLYGONS );
        polygons = get_polygons_ptr( object );
        initialize_polygons( polygons, WHITE, NULL );
        polygons->n_points = 4;
        ALLOC( polygons->points, polygons->n_points );
        ALLOC( polygons->normals, polygons->n_points );
        a1 = (axis + 1) % N_DIMENSIONS;
        a2 = (axis + 2) % N_DIMENSIONS;
        fill_Vector( polygons->normals[0], 0.0, 0.0, 0.0 );
        Vector_coord( polygons->normals[0], axis ) = (Point_coord_type) 1.0;
        polygons->normals[1] = polygons->normals[0];
        polygons->normals[2] = polygons->normals[0];
        polygons->normals[3] = polygons->normals[0];
        Point_coord( polygons->points[0], axis ) = (Point_coord_type) pos;
        Point_coord( polygons->points[0], a1 ) = (Point_coord_type) x_min;
        Point_coord( polygons->points[0], a2 ) = (Point_coord_type) y_min;
        Point_coord( polygons->points[1], axis ) = (Point_coord_type) pos;
        Point_coord( polygons->points[1], a1 ) = (Point_coord_type) x_max;
        Point_coord( polygons->points[1], a2 ) = (Point_coord_type) y_min;
        Point_coord( polygons->points[2], axis ) = (Point_coord_type) pos;
        Point_coord( polygons->points[2], a1 ) = (Point_coord_type) x_max;
        Point_coord( polygons->points[2], a2 ) = (Point_coord_type) y_max;
        Point_coord( polygons->points[3], axis ) = (Point_coord_type) pos;
        Point_coord( polygons->points[3], a1 ) = (Point_coord_type) x_min;
        Point_coord( polygons->points[3], a2 ) = (Point_coord_type) y_max;
        polygons->n_items = 1;
        ALLOC( polygons->end_indices, 1 );
        polygons->end_indices[0] = 4;
        ALLOC( polygons->indices, 4 );
        polygons->indices[0] = 0;
        polygons->indices[1] = 1;
        polygons->indices[2] = 2;
        polygons->indices[3] = 3;
    }

    if( output_graphics_file( output_filename, BINARY_FORMAT, 1, &object )
        != OK )
        return( 1 );

    return( 0 );
}
