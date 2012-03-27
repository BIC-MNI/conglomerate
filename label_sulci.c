#include  <volume_io.h>
#include  <bicpl.h>

private  int  surface_fill(
    polygons_struct   *surface,
    int               n_neighbours[],
    int               *neighbours[],
    Point             *point,
    Real              values[],
    Real              min_value,
    Real              max_value,
    Real              value_to_set,
    Real              max_dist );

int  main(
    int    argc,
    char   *argv[] )
{
    FILE                 *file;
    STRING               surface_filename, sulcus_filename;
    STRING               input_values_filename, output_values_filename;
    int                  i, p, n_objects, n_points;
    int                  *n_neighbours, **neighbours, n_set;
    Point                *points;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *surface;
    Real                 min_value, max_value, *values, value_to_set;
    Real                 max_dist;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &sulcus_filename ) ||
        !get_string_argument( NULL, &input_values_filename ) ||
        !get_real_argument( 0.0, &min_value ) ||
        !get_real_argument( 0.0, &max_value ) ||
        !get_real_argument( 0.0, &value_to_set ) ||
        !get_real_argument( 0.0, &max_dist ) ||
        !get_string_argument( NULL, &output_values_filename ) )
    {
        print_error( "Usage: %s  surface.obj sulcal_points.obj values.txt\n",
                     argv[0] );
        print_error( "           min max value_to_set output.txt\n" );
        return( 1 );
    }

    if( min_value <= value_to_set && value_to_set <= max_value )
    {
        print_error( "The value to set must not be in the min-max range.\n" );
        return( 1 );
    }

    if( input_graphics_file( surface_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
        return( 1 );

    surface = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( sulcus_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( open_file( input_values_filename, READ_FILE, ASCII_FORMAT, &file )!=OK )
        return( 1 );

    ALLOC( values, surface->n_points );

    for_less( p, 0, surface->n_points )
    {
        if( input_real( file, &values[p] ) != OK )
        {
            print_error( "Could not read %d'th value from file.\n", p );
            return( 1 );
        }
    }

    (void) close_file( file );

    create_polygon_point_neighbours( surface, TRUE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    create_polygons_bintree( surface,
                             ROUND( (Real) surface->n_items * 0.1 ) );

    n_set = 0;
    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( object_list[i], &points );

        for_less( p, 0, n_points )
        {
            n_set += surface_fill( surface, n_neighbours, neighbours,
                                   &points[p],
                                   values, min_value, max_value, value_to_set,
                                   max_dist );
        }
    }

    print( "Set %d nodes.\n", n_set );

    if( open_file( output_values_filename, WRITE_FILE, ASCII_FORMAT, &file )
              !=OK )
        return( 1 );

    for_less( p, 0, surface->n_points )
    {
        if( output_real( file, values[p] ) != OK ||
            output_newline( file ) != OK )
        {
            print_error( "Could not write %d'th value from file.\n", p );
            return( 1 );
        }
    }

    (void) close_file( file );

    return( 0 );
}

private  int  surface_fill(
    polygons_struct   *surface,
    int               n_neighbours[],
    int               *neighbours[],
    Point             *point,
    Real              values[],
    Real              min_value,
    Real              max_value,
    Real              value_to_set,
    Real              max_dist )
{
    int                     size, poly, vertex, neigh, point_index, n;
    int                     n_changed;
    int                     step, next_step_index, max_steps;
    Point                   poly_point;
    BOOLEAN                 new_step;
    QUEUE_STRUCT( int )     queue;

    poly = find_closest_polygon_point( point, surface, &poly_point );

    size = GET_OBJECT_SIZE( *surface, poly );

    INITIALIZE_QUEUE( queue );

    n_changed = 0;
    next_step_index = -1;

    for_less( vertex, 0, size )
    {
        point_index = surface->indices[POINT_INDEX(surface->end_indices,poly,
                                                   vertex)];

        if( min_value <= values[point_index] &&
                         values[point_index] <= max_value )
        {
            if( next_step_index < 0 )
                next_step_index = point_index;
            INSERT_IN_QUEUE( queue, point_index );
            values[point_index] = value_to_set;
            ++n_changed;
        }
    }

    step = 0;
    max_steps = ROUND( max_dist );
    new_step = FALSE;

    while( !IS_QUEUE_EMPTY(queue) )
    {
        REMOVE_FROM_QUEUE( queue, point_index );
        if( point_index == next_step_index )
        {
            if( step >= max_steps )
                break;
            ++step;
            new_step = TRUE;
        }

        for_less( n, 0, n_neighbours[point_index] )
        {
            neigh = neighbours[point_index][n];
            if( min_value <= values[neigh] &&
                             values[neigh] <= max_value )
            {
                if( new_step )
                {
                    new_step = FALSE;
                    next_step_index = neigh;
                }

                INSERT_IN_QUEUE( queue, neigh );
                values[neigh] = value_to_set;
                ++n_changed;
            }
        }
    }

    DELETE_QUEUE( queue );

    return( n_changed );
}
