#include  <bicpl.h>

#define  BINTREE_FACTOR   0.4

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               surface_filename, filename, *filenames;
    STRING               lines_filename;
    int                  n_objects, max_size;
    int                  n_values, n_to_do;
    int                  v, poly_size, poly, step, n_steps, line, size;
    int                  p1, p2, vertex;
    Real                 *weights, ratio, *probabilities, scan_step;
    Real                 interval;
    Point                *poly_points, polygon_point, p;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;
    lines_struct         *lines;
    Real                 *values, **components;
    int                  comp, max_index, n_components;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &lines_filename ) ||
        !get_real_argument( 0.0, &scan_step ) )
    {
        print_error( "Usage: %s  surface.obj output_values.mnc step [prob1.mnc]\n",
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

    if( input_graphics_file( lines_filename, &format, &n_objects,
                             &object_list ) != OK )
    {
        print_error( "Error in file: %s\n", lines_filename );
        return( 1 );
    }

    if( n_objects == 0 || get_object_type(object_list[0]) != LINES )
    {
        lines = get_lines_ptr( create_object( LINES ) );
        initialize_lines( lines, WHITE );
    }
    else
        lines = get_lines_ptr( object_list[0] );

    create_polygons_bintree( polygons,
                             ROUND((Real) polygons->n_items * BINTREE_FACTOR ));

    n_components = 0;
    components = NULL;
    filenames = NULL;

    while( get_string_argument( NULL, &filename ) )
    {
        if( input_texture_values( filename, &n_values, &values ) != OK ||
            n_values != polygons->n_points )
        {
            print_error( "Error in values file: %s\n", filename );
            return( 1 );
        }

        ADD_ELEMENT_TO_ARRAY( components, n_components, values, 1 );
        --n_components;
        ADD_ELEMENT_TO_ARRAY( filenames, n_components, filename, 1 );
    }

    max_size = 0;
    for_less( poly, 0, polygons->n_items )
    {
        if( GET_OBJECT_SIZE( *polygons, poly ) > max_size )
            max_size = GET_OBJECT_SIZE( *polygons, poly );
    }

    ALLOC( poly_points, max_size );
    ALLOC( weights, max_size );

    ALLOC( probabilities, n_components );
    for_less( comp, 0, n_components )
        probabilities[comp] = 0.0;

    for_less( line, 0, lines->n_items )
    {
        size = GET_OBJECT_SIZE( *lines, line );
        for_less( v, 0, size-1 )
        {
            p1 = lines->indices[POINT_INDEX(lines->end_indices,line,v)];
            p2 = lines->indices[POINT_INDEX(lines->end_indices,line,v+1)];

            interval = distance_between_points( &lines->points[p1],
                                                &lines->points[p2] );

            n_steps = ROUND( interval / scan_step );
            if( v < size-2 )
                n_to_do = n_steps;
            else
                n_to_do = n_steps+1;

            for_less( step, 0, n_to_do )
            {
                ratio = (Real) step / (Real) n_steps;
                INTERPOLATE_POINTS( p, lines->points[p1], lines->points[p2],
                                    ratio );

                poly = find_closest_polygon_point( &p, polygons,
                                                   &polygon_point );

                poly_size = get_polygon_points( polygons, poly, poly_points );
                get_polygon_interpolation_weights( &polygon_point, poly_size,
                                                   poly_points, weights );

                for_less( comp, 0, n_components )
                {
                    for_less( vertex, 0, poly_size )
                        probabilities[comp] += weights[vertex] *
                              components[comp][
                               polygons->indices[POINT_INDEX(
                                      polygons->end_indices,poly,vertex)]];
                }
            }
        }
    }

    max_index = -1;
    for_less( comp, 0, n_components )
    {
        if( max_index < 0 ||
            probabilities[comp] > probabilities[max_index] )
        {
            max_index = comp;
        }

        print( "%s: %g\n", filenames[comp], probabilities[comp] );
    }

    print( "\n" );

    if( probabilities[max_index] == 0.0 )
        print( "Class: -1 0 none\n" );
    else
        print( "Class: %d %g %s\n", max_index, probabilities[max_index], filenames[max_index] );

    return( 0 );
}
