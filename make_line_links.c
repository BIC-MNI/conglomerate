#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input_lines1.obj  input_lines2.obj output_lines.obj n_links\n\
\n\
     Creates a set of line segments linking the two input lines.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               src1_filename, src2_filename, dest_filename;
    File_formats         format;
    int                  n_objects1, n_objects2, n_links, i, obj_index;
    object_struct        **objects1, **objects2, *object;
    lines_struct         *lines1, *lines2, *lines;
    Point                point1, point2;
    Real                 length;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src1_filename ) ||
        !get_string_argument( NULL, &src2_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_int_argument( 0, &n_links ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src1_filename,
                             &format, &n_objects1, &objects1 ) != OK ||
        n_objects1 != 1 || get_object_type(objects1[0]) != LINES )
    {
        print( "Expected one lines object in file: %s\n", src1_filename );
        return( 1 );
    }

    lines1 = get_lines_ptr( objects1[0] );

    if( lines1->n_items != 1 )
    {
        print( "Expected one lines item in file: %s\n", src1_filename );
        return( 1 );
    }

    if( input_graphics_file( src2_filename,
                             &format, &n_objects2, &objects2 ) != OK ||
        n_objects2 != 1 || get_object_type(objects2[0]) != LINES )
    {
        print( "Expected one lines object in file: %s\n", src2_filename );
        return( 1 );
    }

    lines2 = get_lines_ptr( objects2[0] );

    if( lines2->n_items != 1 )
    {
        print( "Expected one lines item in file: %s\n", src2_filename );
        return( 1 );
    }

    object = create_object( LINES );
    lines = get_lines_ptr( object );
    initialize_lines( lines, WHITE );

    length = get_lines_length( lines1 );
    for_less( i, 0, n_links )
    {
        get_lines_arc_point( lines1, (Real) i / (Real) n_links * length,
                             &point1 );

        (void) find_closest_point_on_object( &point1, objects2[0],
                                             &obj_index, &point2 );

        start_new_line( lines );
        add_point_to_line( lines, &point1 );
        add_point_to_line( lines, &point2 );
    }

    if( output_graphics_file( dest_filename, format, 1, &object ) != OK )
        return( 1 );

    delete_object_list( n_objects1, objects1 );

    return( 0 );
}

private  void  reparameterize_lines(
    lines_struct  *lines,
    lines_struct  *new_lines,
    int           n_points )
{
    int      size, i;
    BOOLEAN  closed_flag;
    Real     total_length, ratio;

    size = GET_OBJECT_SIZE( *lines, 0 );

    closed_flag = lines->indices[0] == lines->indices[size-1];

    initialize_lines_with_size( new_lines, lines->colours[0],
                                n_points, closed_flag );

    total_length = get_lines_length( lines );

    for_less( i, 0, n_points )
    {
        if( closed_flag )
            ratio = (Real) i / (Real) n_points;
        else
            ratio = (Real) i / (Real) (n_points-1);

        get_lines_arc_point( lines, ratio * total_length,
                             &new_lines->points[i] );
    }
}
