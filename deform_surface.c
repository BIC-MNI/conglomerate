#include  <def_mni.h>
#include  <def_module.h>

private  void  usage(
    char  executable[] )
{
    (void) fprintf( stderr, "%s  volume_filename\n", executable );
    (void) fprintf( stderr, "   activity_filename|none   nx ny nz\n" );
    (void) fprintf( stderr, "   input_polygons output_polygons\n");
    (void) fprintf( stderr, "   model_weight model_filename|avg|none\n" );
    (void) fprintf( stderr, "   min_curvature max_curvature\n" );
    (void) fprintf( stderr, "   fract_step max_step\n" );
    (void) fprintf( stderr, "   max_search_distance search_increment\n" );
    (void) fprintf( stderr, "   min_size min_isovalue max_isovalue +/-/n\n" );
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
    char              *model_filename, *normal_direction;
    int               nx, ny, nz;
    deform_struct     deform;
    FILE              *file;
    File_formats      file_format;
    int               n_objects;
    object_struct     **object_list;
    Volume            volume, tmp;
    polygons_struct   *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &activity_filename ) ||
        !get_int_argument( 0, &nx ) ||
        !get_int_argument( 0, &ny ) ||
        !get_int_argument( 0, &nz ) ||
        !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &deform.model_weight ) ||
        !get_string_argument( "", &model_filename ) ||
        !get_real_argument( 0.0, &deform.deformation_model.min_curvature_offset ) ||
        !get_real_argument( 0.0, &deform.deformation_model.max_curvature_offset ) ||
        !get_real_argument( 0.0, &deform.fractional_step ) ||
        !get_real_argument( 0.0, &deform.max_step ) ||
        !get_real_argument( 0.0, &deform.max_search_distance ) ||
        !get_real_argument( 0.0, &deform.search_increment ) ||
        !get_real_argument( 0.0, &deform.min_size ) ||
        !get_real_argument( 0.0, &deform.boundary_definition.min_isovalue ) ||
        !get_real_argument( 0.0, &deform.boundary_definition.max_isovalue ) ||
        !get_string_argument( "", &normal_direction ) ||
        !get_int_argument( 0, &deform.max_iterations ) ||
        !get_real_argument( 0.0, &deform.stop_threshold ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    switch( normal_direction[0] )
    {
    case '-':
        deform.boundary_definition.normal_direction = TOWARDS_LOWER;   break;
    case '+':
        deform.boundary_definition.normal_direction = TOWARDS_HIGHER;  break;
    default:
        deform.boundary_definition.normal_direction = ANY_DIRECTION;   break;
    }

    deform.boundary_definition.variable_threshold_flag = FALSE;
    deform.boundary_definition.gradient_flag = FALSE;
    deform.deform_data.type = VOLUME_DATA;

    deform.deformation_model.position_constrained = FALSE;

    status = input_volume( volume_filename, &volume );

    if( strcmp( activity_filename, "none" ) != 0 )
    {
        alloc_auxiliary_data( volume );

        status = open_file( activity_filename, READ_FILE, BINARY_FORMAT,
                            &file );

        if( status == OK )
            status = io_volume_auxiliary_bit( file, READ_FILE,
                                              volume, ACTIVE_BIT );

        status = close_file( file );
    }

    if( nx > 0 && ny > 0 && nz > 0 )
    {
        tmp = smooth_resample_volume( volume, nx, ny, nz );

        delete_volume( volume );

        volume = tmp;
    }

    deform.deform_data.volume = volume;

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
    {
        polygons = get_polygons_ptr( object_list[0] );

        if( strcmp( model_filename, "none" ) == 0 )
        {
            deform.deformation_model.model_type = POINT_SPHERE_MODEL;
        }
        else if( strcmp( model_filename, "avg" ) == 0 )
        {
            deform.deformation_model.model_type = AVERAGE_MODEL;
        }
        else
        {
            status = input_deformation_model( polygons->n_points,
                                              model_filename,
                                              &deform.deformation_model );
        }
    }

    if( status == OK )
    {
        start_time = current_cpu_seconds();

        deform_polygons( polygons, &deform );
    }

    if( status == OK )
        compute_polygon_normals( polygons );

    if( status == OK )
    {
        end_time = current_cpu_seconds();

        status = open_file( output_filename, WRITE_FILE, BINARY_FORMAT,
                            &file );
    }

    if( status == OK )
        status = io_polygons( file, WRITE_FILE, BINARY_FORMAT, polygons );


    if( status == OK )
        status = close_file( file );

/*
    print_time( "Total cpu time: %g %s\n", end_time - start_time );
*/

    return( status != OK );
}
