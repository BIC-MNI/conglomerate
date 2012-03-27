#include  <volume_io.h>
#include  <bicpl.h>

private  void  create_tristrip(
    polygons_struct  *polygons,
    int              start_poly );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, output_filename;
    int              n_objects, start_poly;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
          "Usage: %s  input.obj  output.obj\n",
                      argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 0, &start_poly );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input_filename );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );

    check_polygons_neighbours_computed( polygons );

    create_tristrip( polygons, start_poly );

    return( 0 );
}

private  void  create_tristrip(
    polygons_struct  *polygons,
    int              start_poly )
{
    int            poly, size, current_poly, current_edge, n_done;
    int            neigh, neigh_edge;
    Smallest_int   *done_flags;

    ALLOC( done_flags, polygons->n_items );

    for_less( poly, 0, polygons->n_items )
        done_flags[poly] = FALSE;

    n_done = 0;
    current_poly = start_poly;
    current_edge = 0;
    size = GET_OBJECT_SIZE( *polygons, current_poly );

    while( TRUE )
    {
        done_flags[current_poly] = TRUE;
        ++n_done;
        neigh = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                                 current_poly,current_edge)];

        if( done_flags[neigh] )
        {
            current_edge = (current_edge - 1 + size) % size;
            neigh = polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                                     current_poly,current_edge)];
            if( done_flags[neigh] )
                break;
        }

        size = GET_OBJECT_SIZE( *polygons, neigh );
        for_less( neigh_edge, 0, size )
        {
            if( polygons->neighbours[POINT_INDEX(polygons->end_indices,
                                         neigh,neigh_edge)] == current_poly )
                break;
        }

        if( neigh_edge >= size )
            handle_internal_error( "create_tristrip" );

        current_poly = neigh;
        current_edge = (neigh_edge - 1 + size) % size;
    }

    print( "%d %d\n", n_done, polygons->n_items );

    FREE( done_flags );
}
