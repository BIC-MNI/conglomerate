#include  <def_mni.h>
#include  <def_module.h>

public  Status  process_object(
    object_struct  *object );

private  void  transform_object(
    Transform      *transform,
    object_struct  *object )
{
    int    i, n_points;
    Point  *points;

    n_points = get_object_points( object, &points );

    for_less( i, 0, n_points )
        transform_point( transform, &points[i], &points[i] );
}

int  main(
    int    argc,
    char   *argv[] )
{
    Status               status;
    Volume               volume;
    char                 *input_filename, *output_filename;
    char                 *volume_filename;
    int                  i, n_objects;
    Transform            transform, scale_transform, translation_transform;
    File_formats         format;
    object_struct        **object_list;
    volume_input_struct  volume_input;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &volume_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );

    status = start_volume_input( volume_filename, FALSE, &volume,
                                 &volume_input );

    make_scale_transform( 1.0, 1.0, 1.0 / 1.5, &scale_transform );
    make_translation_transform( 0.0, 0.0, -25.0, &translation_transform );

    make_identity_transform( &transform );
    concat_transforms( &transform, &transform, &scale_transform );
    concat_transforms( &transform, &transform, &translation_transform );
    concat_transforms( &transform, &transform,
                       &volume->voxel_to_world_transform );

    status = input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list );

    if( status == OK )
        print( "Objects input.\n" );

    if( status == OK )
    {
        for_less( i, 0, n_objects )
        {
            if( status == OK )
                transform_object( &transform, object_list[i] );
        }

        if( status == OK )
            print( "Objects processed.\n" );
    }

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       n_objects, object_list );

    if( status == OK )
        delete_object_list( n_objects, object_list );

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}
