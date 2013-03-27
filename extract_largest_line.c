#include  <volume_io.h>
#include  <bicpl.h>

static  void  extract_largest_line(
    lines_struct  *lines,
    lines_struct  *new_lines );

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
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
    VIO_STR               src_filename, dest_filename;
    VIO_File_formats         format;
    int                  n_objects;
    object_struct        **objects;
    lines_struct         *lines, new_lines;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_filename,
                             &format, &n_objects, &objects ) != VIO_OK ||
        n_objects != 1 || get_object_type(objects[0]) != LINES )
    {
        print( "Expected one lines object in file: %s\n",
               src_filename );
        return( 1 );
    }

    lines = get_lines_ptr( objects[0] );

    extract_largest_line( lines, &new_lines );

    delete_lines( lines );

    *lines = new_lines;

    if( output_graphics_file( dest_filename, format, n_objects, objects ) != VIO_OK)
        return( 1 );

    delete_object_list( n_objects, objects );

    return( 0 );
}

static  void  extract_largest_line(
    lines_struct  *lines,
    lines_struct  *new_lines )
{
    int    size, biggest, index, *new_ids, l, point, vertex, old_index;

    *new_lines = *lines;
    new_lines->n_points = 0;
    new_lines->n_items = 0;
    ALLOC( new_lines->colours, 1 );
    new_lines->colours[0] = lines->colours[0];

    biggest = 0;
    for_less( l, 0, lines->n_items )
    {
        size = GET_OBJECT_SIZE( *lines, l );
        if( l == 0 || size > biggest )
        {
            biggest = size;
            index = l;
        }
    }

    ALLOC( new_ids, lines->n_points );
    for_less( point, 0, lines->n_points )
        new_ids[point] = -1;

    new_lines->n_items = 1;
    ALLOC( new_lines->end_indices, new_lines->n_items );
    new_lines->end_indices[0] = biggest;
    ALLOC( new_lines->indices, biggest );

    for_less( vertex, 0, biggest )
    {
        old_index =lines->indices[POINT_INDEX(lines->end_indices,index,vertex)];
        if( new_ids[old_index] < 0 )
        {
            new_ids[old_index] = new_lines->n_points;
            ADD_ELEMENT_TO_ARRAY( new_lines->points, new_lines->n_points,
                                  lines->points[old_index], DEFAULT_CHUNK_SIZE);
        }

        new_lines->indices[vertex] = new_ids[old_index];
    }

    FREE( new_ids );
}
