#include  <volume_io.h>
#include  <deform.h>

private  void  usage(
    STRING  executable )
{
    print_error( "%s  volume_filename\n", executable );
    print_error( "   activity_filename|none   nx ny nz\n" );
    print_error( "   input_polygons output_polygons\n");
    print_error( "   original_positions|none max_distance\n" );
    print_error( "   n_models\n");
    print_error( "   up_to_n_points model_weight model_filename|avg|none\n" );
    print_error( "   min_curvature max_curvature\n" );
    print_error( "   [up_to_n_points model_weight model_filename|avg|none\n" );
    print_error( "   min_curvature max_curvature]\n" );
    print_error( "   fract_step max_step\n" );
    print_error( "   max_search_distance degrees_continuity\n" );
    print_error( "   min_isovalue max_isovalue +/-/n\n" );
    print_error( "   gradient_threshold angle tolerance\n" );
    print_error( "   max_iterations  movement_threshold stop_threshold\n" );
}

int  main(
    int    argc,
    char   *argv[] )
{
    Status            status;
    STRING            volume_filename, activity_filename;
    STRING            input_filename, output_filename;
    STRING            model_filename, normal_direction, original_filename;
    Real              min_isovalue, max_isovalue, gradient_threshold;
    Real              model_weight, min_curvature_offset, max_curvature_offset;
    Real              angle, tolerance, max_distance;
    Real              separations[N_DIMENSIONS];
    Real              x_filter_width, y_filter_width, z_filter_width;
    int               i, n_models, up_to_n_points;
    deform_struct     deform;
    File_formats      file_format;
    int               n_objects;
    object_struct     **object_list;
    Volume            volume, label_volume, tmp;
    polygons_struct   *polygons;

    set_alloc_checking( FALSE );

    initialize_argument_processing( argc, argv );

    initialize_deformation_parameters( &deform );

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
        !get_int_argument( 0, &deform.degrees_continuity ) ||
        !get_real_argument( 0.0, &min_isovalue ) ||
        !get_real_argument( 0.0, &max_isovalue ) ||
        !get_string_argument( "", &normal_direction ) ||
        !get_real_argument( 0.0, &gradient_threshold ) ||
        !get_real_argument( 0.0, &angle ) ||
        !get_real_argument( 0.0, &tolerance ) ||
        !get_int_argument( 0, &deform.max_iterations ) ||
        !get_real_argument( 0.0, &deform.movement_threshold ) ||
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

    if( x_filter_width > 0.0 && y_filter_width > 0.0 && z_filter_width > 0.0 )
    {
        get_volume_separations( volume, separations );

        x_filter_width /= FABS( separations[X] );
        y_filter_width /= FABS( separations[Y] );
        z_filter_width /= FABS( separations[Z] );

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

    if( status == OK && !equal_strings( original_filename, "none" ) )
    {
        status = input_original_positions( &deform.deformation_model,
                                           original_filename,
                                           max_distance,
                                           polygons->n_points );
    }

    if( status == OK )
        deform_polygons( polygons, &deform );

    if( status == OK )
        compute_polygon_normals( polygons );

    if( status == OK )
        status = output_graphics_file( output_filename, BINARY_FORMAT,
                                       n_objects, object_list );

    return( status != OK );
}
