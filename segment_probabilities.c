#include  <bicpl.h>

#define  GRAY_STRING       "gray"
#define  HOT_STRING        "hot"
#define  SPECTRAL_STRING   "spectral"
#define  RED_STRING        "red"
#define  GREEN_STRING      "green"
#define  BLUE_STRING       "blue"

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               surface_filename, output_filename, filename;
    STRING               output_lines_filename;
    int                  point, n_objects;
    int                  n, *n_neighbours, **neighbours, n_values;
    File_formats         format;
    object_struct        **object_list, *object;
    polygons_struct      *polygons;
    lines_struct         *lines;
    Real                 *values, **components;
    int                  *which_class, comp, max_index, n_components;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &output_lines_filename ) )
    {
        print_error( "Usage: %s  surface.obj output_values.mnc lines.obj [value1] ...\n",
                     argv[0] );
        return( 1 );
    }

    if( input_graphics_file( surface_filename, &format, &n_objects,
                             &object_list ) != OK ||
        n_objects < 1 || get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error in file: %s\n", surface_filename );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );
    n_components = 0;
    components = NULL;

    while( get_string_argument( NULL, &filename ) )
    {
        if( input_texture_values( filename, &n_values, &values ) != OK ||
            n_values != polygons->n_points )
        {
            print_error( "Error in values file: %s\n", filename );
            return( 1 );
        }

        ADD_ELEMENT_TO_ARRAY( components, n_components, values, 1 );
    }

    ALLOC( which_class, polygons->n_points );
    for_less( point, 0, polygons->n_points )
    {
        max_index = -1;
        for_less( comp, 0, n_components )
        {
            if( components[comp][point] > 0.0 &&
                (max_index < 0 ||
                 components[comp][point] > components[max_index][point]) )
            {
                max_index = comp;
            }
        }

        which_class[point] = max_index;
    }

    for_less( comp, 0, n_components )
    {
        FREE( components[comp] );
    }

    FREE( components );

    create_polygon_point_neighbours( polygons, TRUE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    ALLOC( values, polygons->n_points );
    for_less( point, 0, polygons->n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            if( which_class[neighbours[point][n]] != which_class[point] )
                break;
        }

        if( n >= n_neighbours[point] )
            values[point] = 0.0;
        else
            values[point] = 1.0;
    }

    FREE( which_class );

    if( output_texture_values( output_filename, BINARY_FORMAT,
                               polygons->n_points, values ) != OK )
        return( 1 );

    object = create_object( LINES );
    lines = get_lines_ptr( object );
    initialize_lines( lines, WHITE );

    for_less( point, 0, polygons->n_points )
    {
        if( values[point] == 0.0 )
            continue;

        for_less( n, 0, n_neighbours[point] )
        {
            if( values[neighbours[point][n]] == 0.0 )
                continue;

            start_new_line( lines );
            add_point_to_line( lines, &polygons->points[point] );
            add_point_to_line( lines, &polygons->points[neighbours[point][n]] );
        }
    }

    (void) output_graphics_file( output_lines_filename, BINARY_FORMAT,
                                 1, &object );


    return( 0 );
}
