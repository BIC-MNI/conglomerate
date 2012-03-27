#include  <volume_io.h>
#include  <bicpl.h>

private  void  reparameterize_lines(
    lines_struct  *lines,
    lines_struct  *new_lines,
    int           n_points );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input_lines.obj  output_lines.obj\n\
\n\
     Copies the input lines to the output lines, but only the largest line\n\
     \n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               src_filename, dest_filename;
    File_formats         format;
    int                  n_objects, n_points;
    object_struct        **objects;
    lines_struct         *lines, new_lines;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_int_argument( 0, &n_points ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_filename,
                             &format, &n_objects, &objects ) != OK ||
        n_objects != 1 || get_object_type(objects[0]) != LINES )
    {
        print( "Expected one lines object in file: %s\n",
               src_filename );
        return( 1 );
    }

    lines = get_lines_ptr( objects[0] );

    if( lines->n_items != 1 )
    {
        print( "Expected one lines item in file: %s\n", src_filename );
        return( 1 );
    }

    reparameterize_lines( lines, &new_lines, n_points );

    delete_lines( lines );

    *lines = new_lines;

    if( output_graphics_file( dest_filename, format, n_objects, objects ) != OK)
        return( 1 );

    delete_object_list( n_objects, objects );

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
