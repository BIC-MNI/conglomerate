#include  <mni.h>
#include  <module.h>

private  void  usage(
    char  executable[] )
{
    (void) fprintf( stderr, "%s  volume_filename\n", executable );
    (void) fprintf( stderr, "   activity_filename|none   nx ny nz\n" );
    (void) fprintf( stderr, "   input_polygons output_polygons\n");
    (void) fprintf( stderr, "   original_positions|none max_distance\n" );
    (void) fprintf( stderr, "   n_models\n");
    (void) fprintf( stderr, "   up_to_n_points model_weight model_filename|avg|none\n" );
    (void) fprintf( stderr, "   min_curvature max_curvature\n" );
    (void) fprintf( stderr, "   [up_to_n_points model_weight model_filename|avg|none\n" );
    (void) fprintf( stderr, "   min_curvature max_curvature]\n" );
    (void) fprintf( stderr, "   fract_step max_step\n" );
    (void) fprintf( stderr, "   max_search_distance degrees_continuity\n" );
    (void) fprintf( stderr, "   min_isovalue max_isovalue +/-/n\n" );
    (void) fprintf( stderr, "   gradient_threshold angle tolerance\n" );
    (void) fprintf( stderr, "   max_iterations  stop_threshold\n" );
}

int  main( argc, argv )
    int    argc;
    char   *argv[];
{
    Status            status;
    Real              start_time, end_time;
    char              *volume_filename, *activity_filename;
    char              *input_filename, *output_filename;
    char              *model_filename, *normal_direction, *original_filename;
    Real              min_isovalue, max_isovalue, gradient_threshold;
    Real              model_weight, min_curvature_offset, max_curvature_offset;
    Real              angle, tolerance, max_distance;
    Real              separations[N_DIMENSIONS];
    Real              x_filter_width, y_filter_width, z_filter_width;
    int               i, n_models, up_to_n_points;
    deform_struct     deform;
    FILE              *file;
    File_formats      file_format;
    int               n_objects;
    object_struct     **object_list;
    Volume            volume, label_volume, tmp;
    polygons_struct   *polygons;

    initialize_argument_processing( argc, argv );

    initialize_deformation_parameters( &deform );
    deform.movement_threshold = 0.01;

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &activity_filename ) ||
        !get_real_argument( 0.0, &x_filter_width ) ||
        !get_real_argument( 0.0, &y_filter_width ) ||
        !get_real_argument( 0.0, &z_filter_width ) ||
        !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_string_argument( "", &original_filename ) ||
        !get_real_argument( 0.0, &max_distance ) ||
        !get_int_argument( 1, &n_models ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    for_less( i, 0, n_models )
    {
        if( !get_int_argument( 0, &up_to_n_points ) ||
            !get_real_argument( 0.0, &model_weight ) ||
            !get_string_argument( "", &model_filename ) ||
            !get_real_argument( 0.0, &min_curvature_offset ) ||
            !get_real_argument( 0.0, &max_curvature_offset ) )
        {
            usage( argv[0] );
            return( 1 );
        }

        if( add_deformation_model( &deform.deformation_model,
                   up_to_n_points, model_weight, model_filename,
                   min_curvature_offset, max_curvature_offset ) != OK )
            return( 1 );
    }

    if( !get_real_argument( 0.0, &deform.fractional_step ) ||
        !get_real_argument( 0.0, &deform.max_step ) ||
        !get_real_argument( 0.0, &deform.max_search_distance ) ||
        !get_int_argument( 0.0, &deform.degrees_continuity ) ||
        !get_real_argument( 0.0, &min_isovalue ) ||
        !get_real_argument( 0.0, &max_isovalue ) ||
        !get_string_argument( "", &normal_direction ) ||
        !get_real_argument( 0.0, &gradient_threshold ) ||
        !get_real_argument( 0.0, &angle ) ||
        !get_real_argument( 0.0, &tolerance ) ||
        !get_int_argument( 0, &deform.max_iterations ) ||
        !get_real_argument( 0.0, &deform.stop_threshold ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    set_boundary_definition( &deform.boundary_definition,
                             min_isovalue, max_isovalue,
                             gradient_threshold, angle, normal_direction[0],
                             tolerance );

    deform.deform_data.type = VOLUME_DATA;

    status = input_volume( volume_filename, 3, XYZ_dimension_names,
                           NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE,
                           &volume, (minc_input_options *) NULL );

    label_volume = (Volume) NULL;

    if( strcmp( activity_filename, "none" ) != 0 )
    {
        label_volume = create_label_volume( volume );

        status = open_file_with_default_suffix( activity_filename, "act",
                                        READ_FILE, BINARY_FORMAT, &file );

        if( status == OK )
            status = io_volume_activity_bit( file, READ_FILE, label_volume );

        status = close_file( file );
    }

    if( x_filter_width > 0.0 && y_filter_width > 0.0 && z_filter_width > 0.0 )
    {
        get_volume_separations( volume, separations );

        x_filter_width /= ABS( separations[X] );
        y_filter_width /= ABS( separations[Y] );
        z_filter_width /= ABS( separations[Z] );

        tmp = create_box_filtered_volume( volume, NC_BYTE, FALSE, 0.0, 0.0,
                                          x_filter_width,
                                          y_filter_width, z_filter_width );

        if( label_volume != (Volume) NULL )
        {
            delete_volume( label_volume );
            label_volume = (Volume) NULL;
        }

        delete_volume( volume );

        volume = tmp;
    }

    deform.deform_data.volume = volume;
    deform.deform_data.label_volume = label_volume;

    if( status == OK )
    {
        status = input_graphics_file( input_filename, &file_format,
                                      &n_objects, &object_list );
    }

    if( status == OK &&
        (n_objects != 1 || object_list[0]->object_type != POLYGONS) )
    {
        (void) fprintf( stderr, "File must contain 1 polygons struct.\n" );
        status = ERROR;
    }

    if( status == OK )
        polygons = get_polygons_ptr( object_list[0] );

    if( status == OK && strcmp( original_filename, "none" ) != 0 )
    {
        status = input_original_positions( &deform.deformation_model,
                                           original_filename,
                                           max_distance,
                                           polygons->n_points );
    }

    if( status == OK )
    {
        start_time = current_cpu_seconds();

        deform_polygons( polygons, &deform );
    }

    if( status == OK )
        compute_polygon_normals( polygons );

    if( status == OK )
        end_time = current_cpu_seconds();

    if( status == OK )
        status = output_graphics_file( output_filename, BINARY_FORMAT,
                                       n_objects, object_list );
/*
    print_time( "Total cpu time: %g %s\n", end_time - start_time );
*/

    return( status != OK );
}
