#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_volume_filename, object_filename;
    STRING               output_filename;
    Volume               volume, label_volume;
    File_formats         format;
    int                  obj, n_objects, scan_value;
    object_struct        **objects;
    Real                 max_distance;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &object_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
           "Usage: %s  volume.mnc  object.obj  output_file.mnc\n", argv[0]);
        return( 1 );
    }

    (void) get_int_argument( 1, &scan_value );
    (void) get_real_argument( 1.0, &max_distance );

    if( input_volume_header_only( input_volume_filename, 3,
                            File_order_dimension_names, &volume, NULL) != OK )
        return( 1 );

    if( input_graphics_file( object_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_BYTE );

    set_all_volume_label_data( label_volume, 0 );

    print( "Scanning\n" );

    for_less( obj, 0, n_objects )
    {
        scan_object_to_volume( objects[obj],
                               volume, label_volume, scan_value, max_distance );
    }

    print( "Done scanning\n" );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, label_volume, "Scanned from .obj\n", NULL );

    return( 0 );
}
