#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  surface.obj  pairs.tag  output.txt [alternate]\n\
\n\
     .\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING              surface_filename, output_filename, tags_filename;
    STRING              output_prefix, dummy;
    File_formats        format;
    int                 n_volumes, n_tag_points, n_objects, tag;
    Real                **tags1, **tags2, u1, v1, u2, v2, x, y, z;
    Real                du, dv;
    FILE                *file1, *file2;
    Point               point, centre, unit_point;
    polygons_struct     *polygons, unit_sphere;
    object_struct       **objects;
    BOOLEAN             rotate_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &tags_filename ) ||
        !get_string_argument( NULL, &output_prefix ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    rotate_flag = get_string_argument( NULL, &dummy );

    if( input_tag_file( tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, NULL, NULL, NULL, NULL ) != OK )
        return( 1 );

    if( n_volumes != 2 )
    {
        print_error( "Tag file must contain sets of two tags\n" );
        return( 1 );
    }

    if( input_graphics_file( surface_filename,
                             &format, &n_objects, &objects ) != OK ||
        n_objects != 1 || get_object_type(objects[0]) != POLYGONS )
    {
        print_error( "Surface file must contain one polygons struct\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( objects[0] );

    create_polygons_bintree( polygons,
                             ROUND( (Real) polygons->n_items * 0.2 ) );

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0, polygons->n_items,
                               &unit_sphere );

    output_filename = concat_strings( output_prefix, ".u" );
    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file1 ) != OK )
        return( 1 );

    output_filename = concat_strings( output_prefix, ".v" );
    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file2 ) != OK )
        return( 1 );

    for_less( tag, 0, n_tag_points )
    {
        fill_Point( point, tags1[tag][X], tags1[tag][Y], tags1[tag][Z] );
        map_point_to_unit_sphere( polygons, &point, &unit_sphere, &unit_point );

        if( rotate_flag )
        {
            x = -RPoint_z( unit_point );
            y = RPoint_y( unit_point );
            z = RPoint_x( unit_point );
        }
        else
        {
            x = RPoint_x( unit_point );
            y = RPoint_y( unit_point );
            z = RPoint_z( unit_point );
        }

        map_sphere_to_uv( x, y, z, &u1, &v1 );

        fill_Point( point, tags2[tag][X], tags2[tag][Y], tags2[tag][Z] );
        map_point_to_unit_sphere( polygons, &point, &unit_sphere, &unit_point );

        if( rotate_flag )
        {
            x = -RPoint_z( unit_point );
            y = RPoint_y( unit_point );
            z = RPoint_x( unit_point );
        }
        else
        {
            x = RPoint_x( unit_point );
            y = RPoint_y( unit_point );
            z = RPoint_z( unit_point );
        }

        map_sphere_to_uv( x, y, z, &u2, &v2 );

        du = u2 - u1;
        if( du > 0.5 )
            du = du - 1.0;
        else if( du < -0.5 )
            du = du + 1.0;

        dv = v2 - v1;

        if( output_real( file1, tags1[tag][X] ) != OK ||
            output_real( file1, tags1[tag][Y] ) != OK ||
            output_real( file1, tags1[tag][Z] ) != OK ||
            output_real( file1, du ) != OK ||
            output_newline( file1 ) != OK ||
            output_real( file2, tags1[tag][X] ) != OK ||
            output_real( file2, tags1[tag][Y] ) != OK ||
            output_real( file2, tags1[tag][Z] ) != OK ||
            output_real( file2, dv ) != OK ||
            output_newline( file2 ) != OK )
            return( 1 );
    }

    (void) close_file( file1 );
    (void) close_file( file2 );

    return( 0 );
}
