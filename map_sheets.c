#include  <internal_volume_io.h>
#include  <bicpl.h>

#define DEBUG
#undef  DEBUG

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s sphere.obj sphere_flat.obj input.obj output_flat.obj output_fixed dist\n\
\n\
     .\n\n";

    print_error( usage_str, executable, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               sphere_filename, sphere_flat_filename, input_filename;
    STRING               output_flat_filename, output_fixed_filename;
    int                  p, n_objects, best_index;
    int                  sphere_vertex, patch_vertex;
    int                  n_fixed, *fixed_indices;
#ifdef DEBUG
    int                  *sphere_indices;
#endif
    Smallest_int         *init_points_done;
    Real                 distance, best_dist;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *sphere, *sphere_flat, *patch;
    Point                *init_points;
    FILE                 *file;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &sphere_filename ) ||
        !get_string_argument( NULL, &sphere_flat_filename ) ||
        !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_flat_filename ) ||
        !get_string_argument( NULL, &output_fixed_filename ) ||
        !get_real_argument( 0.0, &distance ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( sphere_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    sphere = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( sphere_flat_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    sphere_flat = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    patch = get_polygons_ptr( object_list[0] );

    create_polygons_bintree( patch, ROUND( (Real) patch->n_items * 0.2 ) );

    ALLOC( init_points, patch->n_points );
    ALLOC( init_points_done, patch->n_points );
#ifdef DEBUG
    ALLOC( sphere_indices, patch->n_points );
#endif
    for_less( patch_vertex, 0, patch->n_points )
    {
        fill_Point( init_points[patch_vertex], 0.0, 0.0, 0.0 );
        init_points_done[patch_vertex] = FALSE;
    }

    n_fixed = 0;
    fixed_indices = NULL;

    initialize_progress_report( &progress, FALSE, sphere->n_points,
                                "Projecting" );

    for_less( sphere_vertex, 0, sphere->n_points )
    { 
        best_dist = 0.0;
        best_index = -1;

        best_dist = find_closest_vertex_on_object(
                      &sphere->points[sphere_vertex], object_list[0],
                      &best_index );

        if( best_dist < distance * distance && !init_points_done[best_index] )
        {
            ADD_ELEMENT_TO_ARRAY( fixed_indices, n_fixed, best_index,
                                  DEFAULT_CHUNK_SIZE );
            init_points[best_index] = sphere_flat->points[sphere_vertex];
#ifdef DEBUG
            sphere_indices[best_index] = sphere_vertex;
#endif
            init_points_done[best_index] = TRUE;
        }

        update_progress_report( &progress, sphere_vertex+1 );
    }

    terminate_progress_report( &progress );

    if( n_fixed < 3 )
    {
        print_error( "Number of fixed vertices is only: %d\n", n_fixed );
        print_error( "Cannot continue.\n" );
        return( 1 );
    }

    print( "Number of fixed vertices is %d\n", n_fixed );

    if( open_file( output_fixed_filename, WRITE_FILE, ASCII_FORMAT, &file )
                           != OK )
        return( 1 );

    for_less( p, 0, n_fixed )
    {
        if( output_int( file, fixed_indices[p] ) != OK ||
            output_newline( file ) != OK )
        {
            print_error( "Error writing fixed file.\n" );
            return( 1 );
        }
    }

    (void) close_file( file );

#ifdef  DEBUG
    {
        object_struct  *object;
        lines_struct   *lines;

        object = create_object( LINES );
        lines = get_lines_ptr( object );
        initialize_lines( lines, RED );

        for_less( p, 0, n_fixed )
        {
            start_new_line( lines );
            add_point_to_line( lines, &patch->points[fixed_indices[p]] );
            add_point_to_line( lines, &sphere->points[
                                      sphere_indices[fixed_indices[p]]] );
        }

        (void) output_graphics_file( "lines.obj", ASCII_FORMAT, 1, &object );
        delete_object( object );
    }

    FREE( sphere_indices );
#endif

    FREE( patch->points );
    patch->points = init_points;
    (void) output_graphics_file( output_flat_filename, format, 1, object_list );

    return( 0 );
}
