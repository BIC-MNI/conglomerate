#include  <bicpl.h>
#include  <internal_volume_io.h>

public  Status  process_object(
    object_struct  *object );

private  void  evaluate_points(
    Volume         volume,
    object_struct  *object )
{
    int    i, n_points;
    Real   val;
    Point  *points;

    n_points = get_object_points( object, &points );

    for_less( i, 0, n_points )
    {
        evaluate_volume_in_world( volume, Point_x(points[i]),
                Point_y(points[i]), Point_z(points[i]), 2, FALSE, 0.0, &val,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );

        print( "%g\n", val );
    }
}

int  main(
    int    argc,
    char   *argv[] )
{
    Volume               volume, tmp;
    char                 *input_filename;
    char                 *volume_filename;
    int                  i, n_objects;
    File_formats         format;
    object_struct        **object_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &input_filename ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        evaluate_points( volume, object_list[i] );
    }

    delete_object_list( n_objects, object_list );

    return( 0 );
}
