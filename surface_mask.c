#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_volume_filename, input_surface_filename;
    STRING               output_volume_filename;
    Real                 set_value, separations[MAX_DIMENSIONS];
    Real                 min_value, max_value, value;
    STRING               history;
    File_formats         format;
    Volume               volume, label_volume;
    int                  x, y, z, n_objects, label, voxel[MAX_DIMENSIONS];
    int                  sizes[MAX_DIMENSIONS];
    int                  range_changed[2][N_DIMENSIONS];
    object_struct        **objects;
    BOOLEAN              set_value_specified;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &input_surface_filename ) ||
        !get_string_argument( NULL, &output_volume_filename ) )
    {
        print_error(
            "Usage: %s  in_volume.mnc  in_surface.obj  out_volume.mnc\n",
               argv[0] );
        print_error( "      [min_value max_value] [set_value]\n" );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_value );
    (void) get_real_argument( min_value - 1.0, &max_value );
    set_value_specified = get_real_argument( 0.0, &set_value );

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_BYTE );

    if( input_graphics_file( input_surface_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects < 1 )
    {
        print( "No objects in %s.\n", input_surface_filename);
        return( 1 );
    }

    get_volume_separations( volume, separations );
    get_volume_sizes( volume, sizes );

    print( "Scanning object to volume.\n" );

    scan_object_to_volume( objects[0], volume, label_volume, 1, 0.0 );

/*
    dilate_voxels_3d( volume, label_volume,
                      1.0, 1.0, 0.0, -1.0, 
                      0.0, 0.0, 0.0, -1.0,
                      1.0, EIGHT_NEIGHBOURS, range_changed );

    dilate_voxels_3d( volume, label_volume,
                      0.0, 0.0, 0.0, -1.0,
                      1.0, 1.0, 0.0, -1.0, 
                      0.0, EIGHT_NEIGHBOURS, range_changed );
*/

    voxel[0] = 0;
    voxel[1] = 0;
    voxel[2] = 0;
    fill_connected_voxels( volume, label_volume, EIGHT_NEIGHBOURS,
                           voxel, 0, 0, 2, 0.0, -1.0, range_changed );

    print( "Masking off external voxels.\n" );

    if( !set_value_specified )
        set_value = get_volume_real_min( volume );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                label = (int) get_volume_real_value( label_volume, x, y, z,
                                                     0, 0 );
                if( label == 2 )
                {
                    if( min_value <= max_value )
                    {
                        value = get_volume_real_value( volume, x, y, z, 0, 0 );
                        if( value < min_value || value > max_value )
                            continue;
                    }

                    set_volume_real_value( volume, x, y, z, 0, 0, set_value );
                }
            }
        }
    }
    
    delete_object_list( n_objects, objects );
    delete_volume( label_volume );

    history = create_string( "Surface masked.\n" );

    (void) output_volume( output_volume_filename, NC_UNSPECIFIED,
                            FALSE, 0.0, 0.0, volume, history,
                            NULL );

    delete_string( history );

    delete_volume( volume );

    return( 0 );
}
